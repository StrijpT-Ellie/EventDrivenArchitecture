import cv2
import multiprocessing as mp
import time
import subprocess
import numpy as np
import random
import os
import mediapipe as mp_solutions
from datetime import datetime
import sys
import signal

class VideoAnimation(mp.Process):
    def __init__(self, person_detected_flag):
        super(VideoAnimation, self).__init__()
        self.pixelated_width, self.pixelated_height = 20, 20
        self.display_width, self.display_height = 400, 400
        self.lower_color = np.array([100, 150, 150])
        self.upper_color = np.array([140, 255, 255])
        self.person_detected_flag = person_detected_flag
        self.running = True
        self.pixel_positions = [(j, i) for i in range(self.pixelated_height) for j in range(self.pixelated_width)]
        self.fgbg = cv2.createBackgroundSubtractorMOG2()
        self.debug_file = "person_detection_time.txt"

        with open(self.debug_file, "w") as f:
            f.write("")

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

    def apply_floating_effect(self, positions):
        if self.motion_detected:
            for index in range(len(positions)):
                x, y = positions[index]
                x += random.choice([-1, 0, 1])
                y += random.choice([-1, 0, 1])
                x = max(0, min(self.pixelated_width - 1, x))
                y = max(0, min(self.pixelated_height - 1, y))
                positions[index] = (x, y)

    def run(self):
        try:
            self.cap = cv2.VideoCapture(0)  # Initialize the video capture
            if not self.cap.isOpened():
                print("[ERROR] Unable to open camera")
                return

            self.canvas = None
            self.long_exposure_frame = None
            self.motion_detected = False
            self.person_detected_time = 0
            self.start_time = None

            while self.running:
                ret, frame = self.cap.read()
                if not ret:
                    print("[ERROR] Failed to capture frame")
                    break

                frame = self.enhance_contrast(frame)
                fgmask = self.fgbg.apply(frame)
                contours, _ = cv2.findContours(fgmask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
                self.motion_detected = any(cv2.contourArea(contour) > 500 for contour in contours)

                if self.motion_detected:
                    if self.start_time is None:
                        self.start_time = time.time()
                    self.person_detected_time = time.time() - self.start_time
                else:
                    self.start_time = None
                    self.person_detected_time = 0

                debug_statement = f'Person detected for: {self.person_detected_time:.2f} seconds'
                print(debug_statement)

                if self.person_detected_time >= 30:
                    self.person_detected_flag.value = True
                    break

                gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                gray_frame = cv2.GaussianBlur(gray_frame, (21, 21), 0)
                hsv_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
                mask = cv2.inRange(hsv_frame, self.lower_color, self.upper_color)
                contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                pixelated = cv2.resize(frame, (self.pixelated_width, self.pixelated_height), interpolation=cv2.INTER_LINEAR)
                pixelated = self.quantize_colors(pixelated)
                if self.canvas is None:
                    self.canvas = np.zeros((self.display_height, self.display_width, 3), dtype=np.uint8)

                circle_frame = np.zeros((self.display_height, self.display_width, 3), dtype=np.uint8)
                self.apply_floating_effect(self.pixel_positions)

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
                    self.running = False
                    break

        finally:
            if hasattr(self, 'cap') and self.cap.isOpened():
                self.cap.release()
            cv2.destroyAllWindows()


class CharacterLoader:
    def __init__(self):
        self.character_index = {}
        self.load_character_index()

    def load_character_index(self):
        dir_path = os.path.dirname(os.path.abspath(__file__))
        font_dir = os.path.join(dir_path, "font")
        files = [f for f in os.listdir(font_dir) if f[-4:] == ".csv"]
        for file in files:
            file_path = os.path.join(font_dir, file)
            data = open(file_path, "r").read()
            data = [[[255., 255., 255.] if c == '1' else [0., 0., 0.] for c in line.split(',')] for line in data.split("\n")]
            self.character_index[chr(int(file[5:-4]))] = (np.array(data), len(data[0]), len(data))

    def get_character(self, character):
        return self.character_index.get(character, None)


class TextWriter:
    def __init__(self, character_loader):
        self.character_loader = character_loader

    def put_text(self, frame, text, start, color=[255, 255, 255]):
        column_start = start[1]
        cursor = start
        char_count = 0
        for c in text:
            if c == "\n" or char_count == 50:
                cursor = [cursor[0] + 6, column_start]
                char_count = 0
                if c == "\n":
                    continue
            letter = self.character_loader.get_character(c)
            if letter:
                letter_image = letter[0] * (np.array(color) / 255)
                frame[cursor[0]:cursor[0] + letter[2], cursor[1]:cursor[1] + letter[1]] = letter_image
                cursor[1] += letter[1] + 1
                char_count += 1


class TextDisplay:
    def __init__(self, text_writer):
        self.text_writer = text_writer

    def display_modes(self, selected_mode=None):
        mat = np.zeros((60, 250, 3))
        modes_text = "art\ngame\nchat"
        color = [255, 255, 255]
        if selected_mode == 1:
            color = [0, 255, 0]
        self.text_writer.put_text(mat, "art", [1, 0], color=color)
        color = [255, 255, 255]
        if selected_mode == 2:
            color = [0, 255, 0]
        self.text_writer.put_text(mat, "game", [7, 0], color=color)
        color = [255, 255, 255]
        if selected_mode == 3:
            color = [0, 255, 0]
        self.text_writer.put_text(mat, "chat", [13, 0], color=color)
        cv2.namedWindow('text', cv2.WINDOW_NORMAL)
        cv2.resizeWindow('text', 1200, 600)
        cv2.imshow("text", mat)


class HandDetector:
    def __init__(self, model_complexity=0, min_detection_confidence=0.5, min_tracking_confidence=0.5):
        self.mp_hands = mp_solutions.solutions.hands
        self.hands = self.mp_hands.Hands(model_complexity=model_complexity,
                                         min_detection_confidence=min_detection_confidence,
                                         min_tracking_confidence=min_tracking_confidence)
        self.mp_drawing = mp_solutions.solutions.drawing_utils
        self.mp_drawing_styles = mp_solutions.solutions.drawing_styles

    def process(self, image):
        image.flags.writeable = False
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        results = self.hands.process(image)
        image.flags.writeable = True
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
        return image, results

    def draw_landmarks(self, image, hand_landmarks):
        self.mp_drawing.draw_landmarks(
            image,
            hand_landmarks,
            self.mp_hands.HAND_CONNECTIONS,
            self.mp_drawing_styles.get_default_hand_landmarks_style(),
            self.mp_drawing_styles.get_default_hand_connections_style())


class FingerCounter:
    @staticmethod
    def count_fingers(results):
        finger_count = 0
        if results.multi_hand_landmarks:
            for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):
                hand_label = handedness.classification[0].label
                hand_landmarks = [(lm.x, lm.y) for lm in hand_landmarks.landmark]
                finger_count += FingerCounter._count_for_hand(hand_label, hand_landmarks)
        return finger_count

    @staticmethod
    def _count_for_hand(hand_label, landmarks):
        count = 0
        if (hand_label == "Left" and landmarks[4][0] > landmarks[3][0]) or \
                (hand_label == "Right" and landmarks[4][0] < landmarks[3][0]):
            count += 1
        for tip, pip in [(8, 6), (12, 10), (16, 14), (20, 18)]:
            if landmarks[tip][1] < landmarks[pip][1]:
                count += 1
        return count


class ModeSelector(mp.Process):
    def __init__(self, selected_mode_pipe):
        super(ModeSelector, self).__init__()
        self.selected_mode_pipe = selected_mode_pipe

    def run(self):
        try:
            self.cap = cv2.VideoCapture(0)  # Initialize the video capture
            if not self.cap.isOpened():
                print("[ERROR] Unable to open camera")
                self.selected_mode_pipe.send(None)
                return

            self.detector = HandDetector()
            self.counter = FingerCounter()
            self.output_file_path = "finger_count_output.txt"
            self.clear_file()
            self.start_time = None

            selected_mode = None
            current_count = None

            while self.cap.isOpened():
                success, image = self.cap.read()
                if not success:
                    print("Ignoring empty camera frame.")
                    continue

                image, results = self.detector.process(image)

                if results.multi_hand_landmarks:
                    for hand_landmarks in results.multi_hand_landmarks:
                        self.detector.draw_landmarks(image, hand_landmarks)

                finger_count = self.counter.count_fingers(results)
                self.write_to_file(finger_count)

                if finger_count in [1, 2, 3]:
                    if current_count == finger_count:
                        if self.start_time and (time.time() - self.start_time) > 6:
                            selected_mode = finger_count
                            break
                    else:
                        current_count = finger_count
                        self.start_time = time.time()
                else:
                    current_count = None
                    self.start_time = None

                cv2.putText(image, str(finger_count), (50, 450), cv2.FONT_HERSHEY_SIMPLEX, 3, (255, 0, 0), 10)
                cv2.imshow('MediaPipe Hands', image)

                display = TextDisplay(TextWriter(CharacterLoader()))
                display.display_modes(selected_mode=finger_count if current_count == finger_count and (time.time() - self.start_time) > 3 else None)

                if cv2.waitKey(5) & 0xFF == 27:
                    break

            self.cap.release()
            cv2.destroyAllWindows()
            self.selected_mode_pipe.send(selected_mode)

        finally:
            if hasattr(self, 'cap') and self.cap.isOpened():
                self.cap.release()
            cv2.destroyAllWindows()

    def clear_file(self):
        open(self.output_file_path, 'w').close()

    def write_to_file(self, finger_count):
        with open(self.output_file_path, "a") as output_file:
            output_file.write(f"{finger_count}\n")
            output_file.flush()


class EventHandler(mp.Process):
    def __init__(self, person_detected_flag):
        super(EventHandler, self).__init__()
        self.person_detected_flag = person_detected_flag
        self.running = True

    def run(self):
        while self.running:
            if self.person_detected_flag.value:
                self.person_detected_flag.value = False  # Reset flag
                print("[DEBUG] Person detected for 30 seconds, switching to mode selection.")
                
                # Create a pipe for mode selection communication
                parent_conn, child_conn = mp.Pipe()
                mode_selector = ModeSelector(child_conn)
                mode_selector.start()
                selected_mode = parent_conn.recv()
                mode_selector.join()

                if selected_mode is not None:
                    print(f"[DEBUG] Mode selected: {selected_mode}")
                    if selected_mode == 2:  # Launch game mode
                        print("[DEBUG] Launching game mode script.")
                        self.current_process = subprocess.Popen(["python3", "brickPongForJetson.py"])

                        # Monitor game activity in the same process
                        self.monitor_game_activity()

                        # After the game process ends
                        self.reset()
                        self.running = False
                        return  # Exit the current instance to restart the process

    def reset(self):
        if hasattr(self, 'current_process') and self.current_process:
            print("[DEBUG] Terminating game mode script due to inactivity.")
            self.current_process.terminate()
            self.current_process.wait()
            self.current_process = None

            # Clear the movement_log.txt file
            print("[DEBUG] Clearing movement_log.txt.")
            open("movement_log.txt", "w").close()

    def monitor_game_activity(self):
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
                print("[DEBUG] No game activity detected for 30 seconds.")
                return

            time.sleep(1)


def run_main_logic():
    person_detected_flag = mp.Value('b', False)

    animation = VideoAnimation(person_detected_flag)
    handler = EventHandler(person_detected_flag)

    animation.start()
    handler.start()

    animation.join()
    handler.join()

def signal_handler(sig, frame):
    print('Exiting...')
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    while True:
        run_main_logic()
        print("[DEBUG] Restarting the main logic")
        time.sleep(5)  # Add a small delay before restarting the script to ensure resources are released
