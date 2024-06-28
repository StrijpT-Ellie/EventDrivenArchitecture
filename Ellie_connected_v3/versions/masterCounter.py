#this master file waits for a person to be recognised
#when the person is recognised it will start the game
#when the game is started it will wait for 10 seconds of inactivity
#then it will relaunch person detection again

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

# Configuration for the Caffe model
PROTOTXT = "model/MobileNetSSD_deploy.prototxt.txt"
MODEL = "model/MobileNetSSD_deploy.caffemodel"
CONFIDENCE = 0.2

# Initialize the list of class labels MobileNet SSD was trained to detect
# Then generate a set of bounding box colors for each class
CLASSES = ["background", "aeroplane", "bicycle", "bird", "boat",
           "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
           "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
           "sofa", "train", "tvmonitor"]
COLORS = np.random.uniform(0, 255, size=(len(CLASSES), 3))

# Load our serialized model from disk
print("[INFO] loading model...")
net = cv2.dnn.readNetFromCaffe(PROTOTXT, MODEL)

# Event flags
person_detected_event = threading.Event()
no_activity_event = threading.Event()

# Global variable to control the main loop
running = True
current_process = None
detection_thread = None

def detect_person():
    global running
    vs = VideoStream(src=0).start()
    time.sleep(2.0)
    fps = FPS().start()

    detection_start_time = None
    person_present = False

    while running:
        frame = vs.read()
        frame = imutils.resize(frame, width=400)
        (h, w) = frame.shape[:2]
        blob = cv2.dnn.blobFromImage(cv2.resize(frame, (300, 300)), 0.007843, (300, 300), 127.5)
        net.setInput(blob)
        detections = net.forward()

        found_person = False

        for i in np.arange(0, detections.shape[2]):
            confidence = detections[0, 0, i, 2]

            if confidence > CONFIDENCE:
                idx = int(detections[0, 0, i, 1])
                if CLASSES[idx] == "person":
                    found_person = True
                    box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
                    (startX, startY, endX, endY) = box.astype("int")
                    label = "{}: {:.2f}%".format(CLASSES[idx], confidence * 100)
                    cv2.rectangle(frame, (startX, startY), (endX, endY), COLORS[idx], 2)
                    y = startY - 15 if startY - 15 > 15 else startY + 15
                    cv2.putText(frame, label, (startX, y), cv2.FONT_HERSHEY_SIMPLEX, 0.5, COLORS[idx], 2)

        if found_person:
            if not person_present:
                detection_start_time = time.time()
                person_present = True
            else:
                elapsed_time = time.time() - detection_start_time
                if elapsed_time > 10:
                    print(f"Person detected for {elapsed_time} seconds.")
                    person_detected_event.set()
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

def monitor_game_activity():
    last_activity_time = time.time()

    while True:
        if not os.path.exists("movement_log.txt"):
            print("[DEBUG] movement_log.txt not found, waiting...")
            time.sleep(1)
            continue

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
            no_activity_event.set()
            return

        time.sleep(1)

def event_handler():
    global current_process, detection_thread, running
    while running:
        if person_detected_event.is_set():
            print("Person detected for more than 10 seconds.")
            person_detected_event.clear()
            
            # Stop person detection
            running = False
            if detection_thread is not None:
                detection_thread.join()
            
            # Launch the game mode script
            print("[DEBUG] Launching game mode script.")
            current_process = subprocess.Popen(["python", "brickPong.py"])

            # Start monitoring game activity in a separate thread
            activity_thread = threading.Thread(target=monitor_game_activity)
            activity_thread.start()
            activity_thread.join()

        if no_activity_event.is_set():
            print("[DEBUG] No activity detected for 10 seconds.")
            no_activity_event.clear()
            
            # Countdown before shutting down the game
            for i in range(30, 0, -1):
                print(f"[DEBUG] No activity detected, shutting down the game in {i} seconds...")
                time.sleep(1)

            # Terminate the game process
            if current_process:
                print("[DEBUG] Terminating game mode script due to inactivity.")
                current_process.terminate()
                current_process.wait()
                current_process = None

            # Restart person detection
            running = True
            detection_thread = threading.Thread(target=detect_person)
            detection_thread.start()

        time.sleep(1)

# Thread setup
detection_thread = threading.Thread(target=detect_person)
event_thread = threading.Thread(target=event_handler)

# Start threads
detection_thread.start()
event_thread.start()

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    running = False
    if current_process:
        current_process.terminate()
        current_process.wait()
    detection_thread.join()
    event_thread.join()
    print("Stopped")

# to run 
# py -3.11 masterCounter.py --prototxt .\model\MobileNetSSD_deploy.prototxt.txt --model .\model\MobileNetSSD_deploy.caffemodel
