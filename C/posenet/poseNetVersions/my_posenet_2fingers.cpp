#include "videoSource.h"
#include "videoOutput.h"
#include "poseNet.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h> // Include for mkfifo

bool signal_recieved = false;

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        LogVerbose("received SIGINT\n");
        signal_recieved = true;
    }
}

int usage()
{
    printf("usage: my_posenet [--help] [--network=NETWORK] ...\n");
    printf("                input_URI [output_URI]\n\n");
    printf("Run pose estimation DNN on a video/image stream.\n");
    printf("See below for additional arguments that may not be shown above.\n\n");
    printf("positional arguments:\n");
    printf("    input_URI       resource URI of input stream  (see videoSource below)\n");
    printf("    output_URI      resource URI of output stream (see videoOutput below)\n\n");

    printf("%s", poseNet::Usage());
    printf("%s", videoSource::Usage());
    printf("%s", videoOutput::Usage());
    printf("%s", Log::Usage());

    return 0;
}

bool detect_two_fingers(const std::vector<poseNet::ObjectPose>& poses)
{
    for (const auto& pose : poses) {
        // Assuming keypoint IDs for fingertips are known, e.g., 7 (index fingertip) and 11 (middle fingertip)
        int index_fingertip = pose.FindKeypoint(7);
        int middle_fingertip = pose.FindKeypoint(11);

        if (index_fingertip >= 0 && middle_fingertip >= 0) {
            const auto& kp_index = pose.Keypoints[index_fingertip];
            const auto& kp_middle = pose.Keypoints[middle_fingertip];

            // Simple distance check for a two-finger gesture
            float distance = sqrt(pow(kp_index.x - kp_middle.x, 2) + pow(kp_index.y - kp_middle.y, 2));
            if (distance > 20 && distance < 60) { // Example thresholds, adjust as necessary
                return true;
            }
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    commandLine cmdLine(argc, argv);

    if (cmdLine.GetFlag("help"))
        return usage();

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        LogError("can't catch SIGINT\n");

    videoSource* input = videoSource::Create(cmdLine, ARG_POSITION(0));
    if (!input) {
        LogError("my_posenet: failed to create input stream\n");
        return 1;
    }

    videoOutput* output = videoOutput::Create(cmdLine, ARG_POSITION(1));
    if (!output) {
        LogError("my_posenet: failed to create output stream\n");
        SAFE_DELETE(input);
        return 1;
    }

    poseNet* net = poseNet::Create(cmdLine);
    if (!net) {
        LogError("my_posenet: failed to initialize poseNet\n");
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        return 1;
    }

    const uint32_t overlayFlags = poseNet::OverlayFlagsFromStr(cmdLine.GetString("overlay", "links,keypoints"));

    /*
     * create named pipe
     */
    const char* pipePath = "/tmp/movement_pipe";
    mkfifo(pipePath, 0666);

    int pipe_fd = open(pipePath, O_WRONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        LogError("my_posenet: failed to open named pipe\n");
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        SAFE_DELETE(net);
        return 1;
    }

    while (!signal_recieved) {
        uchar3* image = NULL;
        int status = 0;

        if (!input->Capture(&image, &status)) {
            if (status == videoSource::TIMEOUT)
                continue;

            break; // EOS
        }

        std::vector<poseNet::ObjectPose> poses;
        if (!net->Process(image, input->GetWidth(), input->GetHeight(), poses, overlayFlags)) {
            LogError("my_posenet: failed to process frame\n");
            continue;
        }

        LogInfo("my_posenet: detected %zu %s(s)\n", poses.size(), net->GetCategory());

        if (detect_two_fingers(poses)) {
            std::ostringstream oss;
            for (const auto& pose : poses) {
                for (const auto& keypoint : pose.Keypoints) {
                    oss << "Pose " << pose.ID << ", Keypoint " << keypoint.ID << ": (" << keypoint.x << ", " << keypoint.y << ")\n";
                }
            }
            std::string data = oss.str();
            write(pipe_fd, data.c_str(), data.size());
        }

        if (output) {
            output->Render(image, input->GetWidth(), input->GetHeight());
            char str[256];
            sprintf(str, "TensorRT %i.%i.%i | %s | Network %.0f FPS", NV_TENSORRT_MAJOR, NV_TENSORRT_MINOR, NV_TENSORRT_PATCH, precisionTypeToStr(net->GetPrecision()), net->GetNetworkFPS());
            output->SetStatus(str);

            if (!output->IsStreaming())
                break;
        }

        net->PrintProfilerTimes();
    }

    LogVerbose("my_posenet: shutting down...\n");

    SAFE_DELETE(input);
    SAFE_DELETE(output);
    SAFE_DELETE(net);

    close(pipe_fd);

    LogVerbose("my_posenet: shutdown complete.\n");
    return 0;
}
