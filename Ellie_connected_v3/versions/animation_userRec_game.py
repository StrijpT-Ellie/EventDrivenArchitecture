#Thi is an attempt to integrate video animation responsive to movement with user recognition
#It doesn't work, cannot process one video signal with openCV and mobile net simultaneously
#Tried to implement threads and some other methods, but it doesn't work
#The code launches animation, it lags and doesn't work 
#However, user recognition keeps working 
#It can launch a game and even shut it down after a certain period of inactivity
#However, then it still probably not gonna work or just not gonna render the animation 

#switched to videoAnimation_personDetector.py which uses detected movement for user detection isntead of mobile net 
#new approach can render animation responsive to movement and simulatneously detect user presence

import cv2
import threading
import time
import subprocess
import numpy as np
import random
import imutils
from imutils.video import FPS
import os
from datetime import datetime
from queue import Queue

class VideoCaptureThread:
    def __init__(self, queue, src=0):
        self.queue = queue
        self.cap = cv2.VideoCapture(src)
        self.running = True

    def start(self):
        threading.Thread(target=self._capture_frames).start()

    def _capture_frames(self):
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                print("[ERROR] Failed to capture frame.")
                break
            if not self.queue.full():
                self.queue.put(frame)
        self.cap.release()

    def stop(self):
        self.running = False

class PersonDetector:
    def __init__(self, queue, prototxt, model, confidence=0.2):
        self.queue = queue
        self.prototxt = prototxt
        self.model = model
        self.confidence = confidence
        self.classes = ["background", "aeroplane", "bicycle", "bird", "boat",
                        "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
                        "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
                        "sofa", "train", "tvmonitor"]
        self.colors = np.random.uniform(0, 255, size=(len(self.classes), 3))
        self.net = cv2.dnn.readNetFromCaffe(self.prototxt, self.model)
        self.person_detected_event = threading.Event()
        self.no_activity_event = threading.Event()
        self.running = True
        self.vs = None

    def start_detection(self):
        threading.Thread(target=self._detect_person).start()

    def _detect_person(self):
        fps = FPS().start()
        detection_start_time = None
        person_present = False

        self.vs = cv2.VideoCapture(0)
        if not self.vs.isOpened():
            print("[ERROR] Could not open video device.")
            return
        time.sleep(2.0)

        while self.running:
            ret, frame = self.vs.read()
            if not ret:
                print("[ERROR] Failed to capture frame.")
                break

            frame = imutils.resize(frame, width=400)
            (h, w) = frame.shape[:2]
            blob = cv2.dnn.blobFromImage(cv2.resize(frame, (300, 300)), 0.007843, (300, 300), 127.5)
            self.net.setInput(blob)
            detections = self.net.forward()

            found_person = False

            for i in np.arange(0, detections.shape[2]):
                confidence = detections[0, 0, i, 2]

                if confidence > self.confidence:
                    idx = int(detections[0, 0, i, 1])
                    if self.classes[idx] == "person":
                        found_person = True
                        box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
                        (startX, startY, endX, endY) = box.astype("int")
                        label = "{}: {:.2f}%".format(self.classes[idx], confidence * 100)
                        cv2.rectangle(frame, (startX, startY), (endX, endY), self.colors[idx], 2)
                        y = startY - 15 if startY - 15 > 15 else startY + 15
                        cv2.putText(frame, label, (startX, y), cv2.FONT_HERSHEY_SIMPLEX, 0.5, self.colors[idx], 2)

            if found_person:
                if not person_present:
                    detection_start_time = time.time()
                    person_present = True
                else:
                    elapsed_time = time.time() - detection_start_time
                    if elapsed_time > 10:
                        print(f"Person detected for {elapsed_time} seconds.")
                        self.person_detected_event.set()
                        self.stop_detection()  # Stop detection after setting the event
            else:
                if person_present:
                    person_present = False
                    detection_start_time = None

            fps.update()

        fps.stop()
        self.vs.release()

    def stop_detection(self):
        self.running = False
        if self.vs is not None:
            self.vs.release()



class GameActivityMonitor:
    def __init__(self, no_activity_event):
        self.no_activity_event = no_activity_event

    def monitor_activity(self):
        while True:
            if not os.path.exists("movement_log.txt"):
                print("[DEBUG] movement_log.txt not found, waiting...")
                time.sleep(1)
                continue

            last_activity_time = time.time()

            while True:
                with open("movement_log.txt", "r") as f:
                    lines = f.readlines()
                    if lines:
                        last_line = lines[-1]
                        print(f"[DEBUG] Last line in movement_log.txt: {last_line.strip()}")
                        try:
                            last_activity_time = datetime.strptime(last_line.split(",")[0].strip(), "%Y-%m-%d %H:%M:%S").timestamp()
                        except ValueError as e:
                            print(f"[DEBUG] Error parsing timestamp: {e}")
                            continue

                current_time = time.time()
                if current_time - last_activity_time > 30:
                    print("[DEBUG] No game activity detected for 10 seconds.")
                    self.no_activity_event.set()
                    return

                time.sleep(1)

class PixelatedDisplay:
    def __init__(self, queue):
        self.queue = queue
        self.pixelated_width, self.pixelated_height = 20, 20
        self.display_width, self.display_height = 400, 400
        self.lower_color = np.array([100, 150, 150])
        self.upper_color = np.array([140, 255, 255])
        self.canvas = None
        self.long_exposure_frame = None
        self.prev_gray = None
        self.motion_detected = False
        self.pixel_positions = [(j, i) for i in range(self.pixelated_height) for j in range(self.pixelated_width)]
        self.running = True

    def enhance_contrast(self, image):
        lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
        l, a, b = cv2.split(lab)
        clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
        cl = clahe.apply(l)
        limg = cv2.merge((cl, a, b))
        final = cv2.cvtColor(limg, cv2.COLOR_LAB2BGR)
        return final

    def quantize_colors(self, image):
        quantized = image.copy()
        for i in range(image.shape[0]):
            for j in range(image.shape[1]):
                pixel = image[i, j]
                r, g, b = pixel[0], pixel[1], pixel[2]
                r = 255 if r > 127 else 0
                g = 255 if g > 127 else 0
                b = 255 if b > 127 else 0
                quantized[i, j] = [r, g, b]
        return quantized

    def apply_floating_effect(self):
        if self.motion_detected:
            for index in range(len(self.pixel_positions)):
                x, y = self.pixel_positions[index]
                x += random.choice([-1, 0, 1])
                y += random.choice([-1, 0, 1])
                x = max(0, min(self.pixelated_width - 1, x))
                y = max(0, min(self.pixelated_height - 1, y))
                self.pixel_positions[index] = (x, y)

    def display_pixelated(self):
        while self.running:
            if not self.queue.empty():
                frame = self.queue.get()
                frame = self.enhance_contrast(frame)
                gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                gray_frame = cv2.GaussianBlur(gray_frame, (21, 21), 0)

                if self.prev_gray is None:
                    self.prev_gray = gray_frame
                    continue

                frame_diff = cv2.absdiff(self.prev_gray, gray_frame)
                threshold_frame = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)[1]
                threshold_frame = cv2.dilate(threshold_frame, None, iterations=2)
                self.motion_detected = np.sum(threshold_frame) > 10000
                self.prev_gray = gray_frame
                hsv_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
                mask = cv2.inRange(hsv_frame, self.lower_color, self.upper_color)
                contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                pixelated = cv2.resize(frame, (self.pixelated_width, self.pixelated_height), interpolation=cv2.INTER_LINEAR)
                pixelated = self.quantize_colors(pixelated)

                if self.canvas is None:
                    self.canvas = np.zeros((self.display_height, self.display_width, 3), dtype=np.uint8)

                circle_frame = np.zeros((self.display_height, self.display_width, 3), dtype=np.uint8)
                self.apply_floating_effect()
                radius = self.display_width // self.pixelated_width // 2
                spacing_x = self.display_width // self.pixelated_width
                spacing_y = self.display_height // self.pixelated_height

                for idx, (x, y) in enumerate(self.pixel_positions):
                    color = pixelated[y, x]
                    center_x = x * spacing_x + spacing_x // 2
                    center_y = y * spacing_y + spacing_y // 2
                    cv2.circle(circle_frame, (center_x, center_y), radius, color.tolist(), -1)

                if self.long_exposure_frame is None:
                    self.long_exposure_frame = np.zeros_like(circle_frame, dtype=np.float32)
                self.long_exposure_frame = cv2.addWeighted(self.long_exposure_frame, 0.95, circle_frame.astype(np.float32), 0.05, 0)
                self.long_exposure_frame = np.roll(self.long_exposure_frame, 1, axis=0)
                long_exposure_frame_8bit = cv2.convertScaleAbs(self.long_exposure_frame)
                combined_frame = cv2.addWeighted(long_exposure_frame_8bit, 0.7, self.canvas, 0.3, 0)
                cv2.imshow('Pixelated', combined_frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

        cv2.destroyAllWindows()

    def stop_display(self):
        self.running = False

class EventHandler:
    def __init__(self, detector, monitor, display):
        self.detector = detector
        self.monitor = monitor
        self.display = display
        self.current_process = None
        self.running = True

    def handle_events(self):
        while self.running:
            if self.detector.person_detected_event.is_set():
                print("Person detected for more than 10 seconds.")
                self.detector.person_detected_event.clear()

                # Stop pixelated display
                self.display.stop_display()

                # Stop the person detection
                self.detector.stop_detection()

                # Ensure the video capture device is released
                if self.detector.vs is not None:
                    self.detector.vs.release()

                # Launch the game mode script
                print("[DEBUG] Launching game mode script.")
                self.current_process = subprocess.Popen(["python", "brickPong.py"])

                # Start monitoring game activity in a separate thread
                activity_thread = threading.Thread(target=self.monitor.monitor_activity)
                activity_thread.start()
                activity_thread.join()

            if self.monitor.no_activity_event.is_set():
                print("[DEBUG] No activity detected for 10 seconds.")
                self.monitor.no_activity_event.clear()

                # Countdown before shutting down the game
                countdown_start_time = time.time()
                while time.time() - countdown_start_time < 30:
                    with open("movement_log.txt", "r") as f:
                        lines = f.readlines()
                        if lines:
                            last_line = lines[-1]
                            print(f"[DEBUG] Last line in movement_log.txt: {last_line.strip()}")
                            try:
                                last_activity_time = datetime.strptime(last_line.split(",")[0].strip(), "%Y-%m-%d %H:%M:%S").timestamp()
                                if time.time() - last_activity_time < 30:
                                    countdown_start_time = time.time()
                                    print("[DEBUG] Movement detected, resetting countdown.")
                            except ValueError as e:
                                print(f"[DEBUG] Error parsing timestamp: {e}")
                                continue
                    print(f"[DEBUG] No activity detected, shutting down the game in {30 - int(time.time() - countdown_start_time)} seconds...")
                    time.sleep(1)

                # Terminate the game process
                if self.current_process:
                    print("[DEBUG] Terminating game mode script due to inactivity.")
                    self.current_process.terminate()
                    self.current_process.wait()
                    self.current_process = None

                    # Clear the movement_log.txt file
                    print("[DEBUG] Clearing movement_log.txt.")
                    open("movement_log.txt", "w").close()

                # Reset the detector state
                self.detector.reset()

                # Restart pixelated display
                display_thread = threading.Thread(target=self.display.display_pixelated)
                display_thread.start()

                # Restart person detection
                self.detector.running = True
                self.detector.start_detection()

            time.sleep(1)

    def start_event_handling(self):
        event_thread = threading.Thread(target=self.handle_events)
        event_thread.start()

def main():
    prototxt = "model/MobileNetSSD_deploy.prototxt.txt"
    model = "model/MobileNetSSD_deploy.caffemodel"

    frame_queue = Queue(maxsize=10)

    video_capture = VideoCaptureThread(queue=frame_queue)
    video_capture.start()

    detector = PersonDetector(queue=frame_queue, prototxt=prototxt, model=model)
    monitor = GameActivityMonitor(detector.no_activity_event)
    display = PixelatedDisplay(queue=frame_queue)
    handler = EventHandler(detector, monitor, display)

    detector.start_detection()
    display_thread = threading.Thread(target=display.display_pixelated)
    display_thread.start()
    handler.start_event_handling()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        video_capture.stop()
        detector.stop_detection()
        display.stop_display()
        handler.running = False
        display_thread.join()
        print("Stopped")

if __name__ == "__main__":
    main()
