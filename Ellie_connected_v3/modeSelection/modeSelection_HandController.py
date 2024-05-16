import cv2
import numpy as np
import os
import mediapipe as mp

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

    def put_text(self, frame, text, start):
        column_start = start[1]
        cursor = start
        char_count = 0
        for c in text:
            if c == "\n" or char_count == 50:
                cursor = [cursor[0]+6, column_start]
                char_count = 0
                if c == "\n":
                    continue
            letter = self.character_loader.get_character(c)
            if letter:
                frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter[0]
                cursor[1] += letter[1]+1
                char_count += 1

class TextDisplay:
    def __init__(self, text_writer):
        self.text_writer = text_writer

    def display_text(self, text):
        mat = np.zeros((40, 250, 3))
        self.text_writer.put_text(mat, text, [1, 0])
        cv2.namedWindow('text', cv2.WINDOW_NORMAL)
        cv2.resizeWindow('text', 1200, 600)
        cv2.imshow("text", mat)
        cv2.waitKey(5000)
        cv2.destroyAllWindows()

class HandDetector:
    def __init__(self, model_complexity=0, min_detection_confidence=0.5, min_tracking_confidence=0.5):
        self.mp_hands = mp.solutions.hands
        self.hands = self.mp_hands.Hands(model_complexity=model_complexity,
                                         min_detection_confidence=min_detection_confidence,
                                         min_tracking_confidence=min_tracking_confidence)
        self.mp_drawing = mp.solutions.drawing_utils
        self.mp_drawing_styles = mp.solutions.drawing_styles

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
        # Thumb
        if (hand_label == "Left" and landmarks[4][0] > landmarks[3][0]) or \
           (hand_label == "Right" and landmarks[4][0] < landmarks[3][0]):
            count += 1
        # Other fingers
        for tip, pip in [(8, 6), (12, 10), (16, 14), (20, 18)]:
            if landmarks[tip][1] < landmarks[pip][1]:
                count += 1
        return count

class App:
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        self.detector = HandDetector()
        self.counter = FingerCounter()
        self.output_file_path = "finger_count_output.txt"
        self.clear_file()

    def clear_file(self):
        """Clears or creates the output file."""
        open(self.output_file_path, 'w').close()

    def run(self):
        selected_mode = None
        with open(self.output_file_path, "a") as self.output_file:  # Open the file in append mode
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
                self.write_to_file(finger_count)  # Write the count to the file and flush

                cv2.putText(image, str(finger_count), (50, 450), cv2.FONT_HERSHEY_SIMPLEX, 3, (255, 0, 0), 10)
                cv2.imshow('MediaPipe Hands', image)

                if finger_count in [1, 2, 3]:  # Select mode based on finger count
                    selected_mode = finger_count
                    break

                if cv2.waitKey(5) & 0xFF == 27:
                    break

            self.cap.release()
            cv2.destroyAllWindows()
        return selected_mode

    def write_to_file(self, finger_count):
        self.output_file.write(f"{finger_count}\n")
        self.output_file.flush()  # Flush after every write

def main():
    loader = CharacterLoader()
    writer = TextWriter(loader)
    display = TextDisplay(writer)
    app = App()

    # Run the app to get the selected mode
    mode = app.run()

    if mode == 1:
        modes_text = "art"
    elif mode == 2:
        modes_text = "game"
    elif mode == 3:
        modes_text = "chat"
    else:
        modes_text = "Invalid selection"

    display.display_text(modes_text)

if __name__ == "__main__":
    main()
