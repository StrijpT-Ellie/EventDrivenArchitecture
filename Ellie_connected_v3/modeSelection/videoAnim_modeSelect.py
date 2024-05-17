#this script works 
#it loads the videoAnimation responsive to movement 
#if person is detected for 30 sec than it switches to mode selection
#mode selection works with gesture recognition and fingere recognition 
#when a finger is show for a certain amount of time it selects the mode
#finger recognition shuts down and it displays the selected mode for a while and shuts down 
#can return the number output in .txt file

import cv2
import numpy as np
import random
import time
import os
import mediapipe as mp

# Pixelated Animation Script
# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Define the dimensions for the enlarged pixelated display
display_width, display_height = 400, 400

# Define the lower and upper bounds for the color to track (in HSV space)
lower_color = np.array([100, 150, 150])  # Adjust these values for your specific color
upper_color = np.array([140, 255, 255])  # Adjust these values for your specific color

# Open the camera
cap = cv2.VideoCapture(0)

# Initialize a canvas for drawing
canvas = None

# Initialize the long exposure frame
long_exposure_frame = None

# Initialize variables for motion detection
ret, prev_frame = cap.read()
prev_gray = cv2.cvtColor(prev_frame, cv2.COLOR_BGR2GRAY)
prev_gray = cv2.GaussianBlur(prev_gray, (21, 21), 0)

motion_detected = False

# Create background subtractor
fgbg = cv2.createBackgroundSubtractorMOG2()

# Counter for person detection time
person_detected_time = 0
start_time = None

# Output file for debug statements
debug_file = "person_detection_time.txt"

# Clean the debug file when the program starts
with open(debug_file, "w") as f:
    f.write("")

# Function to enhance contrast
def enhance_contrast(image):
    lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
    cl = clahe.apply(l)
    limg = cv2.merge((cl, a, b))
    final = cv2.cvtColor(limg, cv2.COLOR_LAB2BGR)
    return final

# Function to quantize colors to the nearest primary RGB values with intensity
def quantize_colors(image):
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

# Initialize pixel positions for floating effect
pixel_positions = [(j, i) for i in range(pixelated_height) for j in range(pixelated_width)]

# Function to apply floating effect
def apply_floating_effect(positions):
    global motion_detected
    if motion_detected:
        for index in range(len(positions)):
            x, y = positions[index]
            # Randomly change position if motion is detected
            x += random.choice([-1, 0, 1])
            y += random.choice([-1, 0, 1])

            # Ensure positions are within bounds
            x = max(0, min(pixelated_width - 1, x))
            y = max(0, min(pixelated_height - 1, y))

            positions[index] = (x, y)

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Enhance the contrast of the frame
    frame = enhance_contrast(frame)

    # Apply background subtractor to detect motion
    fgmask = fgbg.apply(frame)
    
    # Find contours in the mask
    contours, _ = cv2.findContours(fgmask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # Check if any contour area is large enough to be considered as motion
    motion_detected = any(cv2.contourArea(contour) > 500 for contour in contours)
    
    # Start the timer if motion is detected
    if motion_detected:
        if start_time is None:
            start_time = time.time()
        person_detected_time = time.time() - start_time
    else:
        start_time = None
        person_detected_time = 0

    # Print the debug statement to the terminal and write to the file
    debug_statement = f'Person detected for: {person_detected_time:.2f} seconds'
    print(debug_statement)
    with open(debug_file, "a") as f:
        f.write(debug_statement + "\n")
    
    # Stop animation and switch mode if person detected for 30 seconds
    if person_detected_time >= 30:
        break

    # Convert frame to grayscale for motion detection
    gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray_frame = cv2.GaussianBlur(gray_frame, (21, 21), 0)

    # Compute the absolute difference between the current frame and the previous frame
    frame_diff = cv2.absdiff(prev_gray, gray_frame)
    threshold_frame = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)[1]
    threshold_frame = cv2.dilate(threshold_frame, None, iterations=2)
    
    # Check if there is any motion detected
    motion_detected = motion_detected or np.sum(threshold_frame) > 10000

    # Update previous frame
    prev_gray = gray_frame

    # Convert frame to HSV color space
    hsv_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Create a mask for the color to track
    mask = cv2.inRange(hsv_frame, lower_color, upper_color)

    # Find contours in the mask
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # If contours are found, get the largest one
    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        (x, y), radius = cv2.minEnclosingCircle(largest_contour)
        center = (int(x), int(y))

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Quantize the colors to primary RGB values
    pixelated = quantize_colors(pixelated)

    # Initialize the canvas if it's None
    if canvas is None:
        canvas = np.zeros((display_height, display_width, 3), dtype=np.uint8)

    # Create a blank frame for drawing circles
    circle_frame = np.zeros((display_height, display_width, 3), dtype=np.uint8)

    # Apply the floating effect
    apply_floating_effect(pixel_positions)

    # Calculate the radius and spacing for the circles
    radius = display_width // pixelated_width // 2
    spacing_x = display_width // pixelated_width
    spacing_y = display_height // pixelated_height

    # Draw circles for each pixel
    for idx, (x, y) in enumerate(pixel_positions):
        color = pixelated[y, x]
        center_x = x * spacing_x + spacing_x // 2
        center_y = y * spacing_y + spacing_y // 2
        cv2.circle(circle_frame, (center_x, center_y), radius, color.tolist(), -1)

    # Add the current frame to the long exposure frame
    if long_exposure_frame is None:
        long_exposure_frame = np.zeros_like(circle_frame, dtype=np.float32)
    long_exposure_frame = cv2.addWeighted(long_exposure_frame, 0.95, circle_frame.astype(np.float32), 0.05, 0)

    # Apply the waterfall effect by shifting the pixels downward
    long_exposure_frame = np.roll(long_exposure_frame, 1, axis=0)

    # Convert the long exposure frame to 8-bit
    long_exposure_frame_8bit = cv2.convertScaleAbs(long_exposure_frame)

    # Combine the canvas with the long exposure frame
    combined_frame = cv2.addWeighted(long_exposure_frame_8bit, 0.7, canvas, 0.3, 0)

    # Display the combined frame
    cv2.imshow('Pixelated', combined_frame)

    # Break the loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# After the loop release the cap object
cap.release()
# Destroy all the windows
cv2.destroyAllWindows()

# Clear the debug file when the program ends
with open(debug_file, "w") as f:
    f.write("")

# Mode Selection Script
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
                cursor = [cursor[0]+6, column_start]
                char_count = 0
                if c == "\n":
                    continue
            letter = self.character_loader.get_character(c)
            if letter:
                letter_image = letter[0] * (np.array(color) / 255)
                frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter_image
                cursor[1] += letter[1]+1
                char_count += 1

class TextDisplay:
    def __init__(self, text_writer):
        self.text_writer = text_writer

    def display_modes(self, selected_mode=None):
        mat = np.zeros((60, 250, 3))
        modes_text = "art\ngame\nchat"
        color = [255, 255, 255]
        if selected_mode == 1:
            color = [0, 255, 0]  # Green for "art"
        self.text_writer.put_text(mat, "art", [1, 0], color=color)
        color = [255, 255, 255]
        if selected_mode == 2:
            color = [0, 255, 0]  # Green for "game"
        self.text_writer.put_text(mat, "game", [7, 0], color=color)
        color = [255, 255, 255]
        if selected_mode == 3:
            color = [0, 255, 0]  # Green for "chat"
        self.text_writer.put_text(mat, "chat", [13, 0], color=color)
        cv2.namedWindow('text', cv2.WINDOW_NORMAL)
        cv2.resizeWindow('text', 1200, 600)
        cv2.imshow("text", mat)

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
        self.start_time = None

    def clear_file(self):
        """Clears or creates the output file."""
        open(self.output_file_path, 'w').close()

    def run(self):
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
            self.write_to_file(finger_count)  # Write the count to the file and flush

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
        return selected_mode

    def write_to_file(self, finger_count):
        with open(self.output_file_path, "a") as output_file:
            output_file.write(f"{finger_count}\n")
            output_file.flush()  # Flush after every write

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

    display.display_modes(selected_mode=mode)
    cv2.waitKey(5000)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
