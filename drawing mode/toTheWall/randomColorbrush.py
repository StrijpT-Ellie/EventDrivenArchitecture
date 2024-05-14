import cv2
import numpy as np
from contourwall import ContourWall
import mediapipe as mp
import random

# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Open the camera
cap = cv2.VideoCapture(0)

# Initialize ContourWall
cw = ContourWall()
cw.single_new_with_port("COM4")

# Initialize MediaPipe hand detection
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(max_num_hands=1)
mp_drawing = mp.solutions.drawing_utils

# Initialize the persistent pixel data for ContourWall
persistent_pixels = np.zeros((pixelated_height, pixelated_width, 3), dtype=np.uint8)

# Function to enhance contrast
def enhance_contrast(image):
    lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(4, 4))
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

# Function to send the pixelated data to ContourWall with persistence
def send_to_contour_wall(cw, persistent_pixels):
    for i in range(pixelated_height):
        for j in range(pixelated_width):
            # Convert BGR to RGB for ContourWall
            color = persistent_pixels[i, j][::-1]
            cw.pixels[i, j] = color
    cw.show()

# Initialize a canvas for drawing
canvas = np.zeros((pixelated_height, pixelated_width, 3), dtype=np.uint8)

# Function to generate a random color
def get_random_color():
    return [random.randint(0, 255) for _ in range(3)]

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Enhance the contrast of the frame
    frame = enhance_contrast(frame)

    # Convert the frame to RGB for MediaPipe
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # Process the frame for hand detection
    results = hands.process(frame_rgb)

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Quantize the colors to primary RGB values
    pixelated = quantize_colors(pixelated)

    # If a hand is detected, draw a trace on the canvas
    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            # Get the palm base landmark (index 0)
            palm_base = hand_landmarks.landmark[0]
            centerX = int(palm_base.x * pixelated_width)
            centerY = int(palm_base.y * pixelated_height)
            # Clamp coordinates to be within bounds
            centerX = max(0, min(pixelated_width - 1, centerX))
            centerY = max(0, min(pixelated_height - 1, centerY))
            # Generate a random color for the brush
            random_color = get_random_color()
            # Draw a fatter brush
            brush_size = 1  # Increase the brush size
            cv2.circle(canvas, (centerX, centerY), brush_size, random_color, -1)
            # Update persistent pixels
            for dy in range(-brush_size, brush_size + 1):
                for dx in range(-brush_size, brush_size + 1):
                    if 0 <= centerY + dy < pixelated_height and 0 <= centerX + dx < pixelated_width:
                        persistent_pixels[centerY + dy, centerX + dx] = random_color

    # Combine the canvas with the pixelated image
    combined = cv2.addWeighted(pixelated, 0.7, canvas, 0.3, 0)

    # Display the combined image on screen
    cv2.imshow('Drawing Mode', combined)

    # Send the persistent pixel data to ContourWall
    send_to_contour_wall(cw, persistent_pixels)

    # Break the loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# After the loop release the cap object
cap.release()
# Destroy all the windows
cv2.destroyAllWindows()

# Clear ContourWall pixels
cw.fill_solid(0, 0, 0)
cw.show()
