import cv2

class VideoAnimation:
    def __init__(self):
        self.pixelated_width, self.pixelated_height = 20, 20
        self.display_width, self.display_height = 400, 400
        self.lower_color = np.array([100, 150, 150])
        self.upper_color = np.array([140, 255, 255])
        self.cap = None

    def open_camera(self):
        cap = cv2.VideoCapture(0)
        if cap.isOpened():
            return cap
        print("[ERROR] Unable to open camera at /dev/video0.")
        return None

    def run(self):
        self.cap = self.open_camera()
        if not self.cap:
            print("[ERROR] Camera could not be opened.")
            return
        
        while True:
            ret, frame = self.cap.read()
            if not ret:
                break

            cv2.imshow('Camera Test', frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        self.cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    animation = VideoAnimation()
    animation.run()
