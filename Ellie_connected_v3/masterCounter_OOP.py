#This is a working prototype of the mode switching based on MobileNet user recognition
#When a person is recognised for 10 sec (+10 sec delay), the game mode is launched
#When no activity is detected for 10 sec (no moving bar), the game mode is terminated
#User recognition begins again and waits for a user to appear

import cv2
import threading
import time
import subprocess
import numpy as np
import imutils
from imutils.video import VideoStream
from imutils.video import FPS
import os
from datetime import datetime

class PersonDetector:
    def __init__(self, prototxt, model, confidence=0.2):
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
        self.detection_thread = None

    def detect_person(self):
        vs = VideoStream(src=0).start()
        time.sleep(2.0)
        fps = FPS().start()

        detection_start_time = None
        person_present = False

        while self.running:
            frame = vs.read()
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
            else:
                if person_present:
                    person_present = False
                    detection_start_time = None

            cv2.imshow("Frame", frame)
            key = cv2.waitKey(1) & 0xFF

            if key == ord("q"):
                break

            fps.update()

        fps.stop()
        vs.stop()
        cv2.destroyAllWindows()

    def start_detection(self):
        self.detection_thread = threading.Thread(target=self.detect_person)
        self.detection_thread.start()

    def stop_detection(self):
        self.running = False
        if self.detection_thread:
            self.detection_thread.join()

    def reset(self):
        self.person_detected_event.clear()
        self.running = True

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

class EventHandler:
    def __init__(self, detector, monitor):
        self.detector = detector
        self.monitor = monitor
        self.current_process = None
        self.running = True

    def handle_events(self):
        while self.running:
            if self.detector.person_detected_event.is_set():
                print("Person detected for more than 10 seconds.")
                self.detector.person_detected_event.clear()

                # Stop person detection
                self.detector.stop_detection()

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

                # Restart person detection
                self.detector.start_detection()

            time.sleep(1)

    def start_event_handling(self):
        event_thread = threading.Thread(target=self.handle_events)
        event_thread.start()

def main():
    prototxt = "model/MobileNetSSD_deploy.prototxt.txt"
    model = "model/MobileNetSSD_deploy.caffemodel"
    
    detector = PersonDetector(prototxt, model)
    monitor = GameActivityMonitor(detector.no_activity_event)
    handler = EventHandler(detector, monitor)

    detector.start_detection()
    handler.start_event_handling()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        detector.running = False
        handler.running = False
        if handler.current_process:
            handler.current_process.terminate()
            handler.current_process.wait()
        detector.stop_detection()
        print("Stopped")

if __name__ == "__main__":
    main()
