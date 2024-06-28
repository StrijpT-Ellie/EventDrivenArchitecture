//this file takes video from default camera and renders it to pixelated array
//made on Apple Silicon, might require some changes to work on other platforms 

//On Jetson Nano 
//Set path 
//include </usr/include/opencv4/opencv2/opencv.hpp>

//Compile 
//g++ -std=c++11 -x c++ -I/usr/include/opencv4 video2Array.c -L/usr/lib/aarch64-linux-gnu -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -o video2Array

//Set path properly (installed opencv with homebrew requires path be like)
//</opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/include/opencv4/opencv2/opencv.hpp>

//cd /opt/homebrew/opt/ffmpeg@6/lib

//Make symbolic links 

//ln -s libavcodec.61.dylib libavcodec.60.dylib
//ln -s libavformat.61.dylib libavformat.60.dylib
//ln -s libavutil.59.dylib libavutil.58.dylib
//ln -s libswscale.8.dylib libswscale.7.dylib
//ln -s libavdevice.61.dylib libavdevice.60.dylib
//ln -s libavfilter.10.dylib libavfilter.9.dylib
//ln -s libswresample.5.dylib libswresample.4.dylib
//ln -s libpostproc.58.dylib libpostproc.57.dylib

//Set paths 
//export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/lib:$DYLD_LIBRARY_PATH
//export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/lib:/opt/homebrew/opt/ffmpeg@6/lib:$DYLD_LIBRARY_PATH

//To compile 
//g++ -std=c++11 -I/opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/include/opencv4 -L/opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -o video2Array video2Array.c

//to avoid setting DYLD_LIBRARY_PATH every time you run your program, add the export command to your shell profile (.bash_profile, .zshrc, etc.):
//echo 'export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/opencv/4.9.0_8.reinstall/lib:/opt/homebrew/opt/ffmpeg@6/lib:$DYLD_LIBRARY_PATH' >> ~/.zshrc
//source ~/.zshrc

#include <stdio.h>
#include </usr/include/opencv4/opencv2/opencv.hpp>

int main(int argc, char** argv) {
    // Open the default camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    cv::Mat frame;
    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Resize the frame to 20x20
        cv::Mat resizedFrame;
        cv::resize(frame, resizedFrame, cv::Size(20, 20), 0, 0, cv::INTER_LINEAR);

        // Resize it back to the original size to visualize the pixelated effect
        cv::Mat pixelatedFrame;
        cv::resize(resizedFrame, pixelatedFrame, frame.size(), 0, 0, cv::INTER_NEAREST);

        // Display the pixelated frame
        cv::imshow("Pixelated Webcam", pixelatedFrame);

        // Exit the loop on 'q' key press
        if (cv::waitKey(30) >= 0) break;
    }

    // Release the camera
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
