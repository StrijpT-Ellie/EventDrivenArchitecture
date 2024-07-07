#Less flickering than in flickeringParticles_grid.py
#Video is projected on pixelated grid, with each pixel being a circle
#The circles flicker when the camera detects motion
#Body area becomes black, while the rest of the image becomes white with some colors

import cv2
import numpy as np
import random

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
original_positions = [(j, i) for i in range(pixelated_height) for j in range(pixelated_width)]
pixel_positions = original_positions.copy()

# Function to apply floating effect
def apply_floating_effect(positions):
    global motion_detected
    if motion_detected:
        for index in range(len(positions)):
            x, y = positions[index]
            # Increase velocity significantly if motion is detected
            x += random.choice([-2, -1, 0, 1, 2])
            y += random.choice([-2, -1, 0, 1, 2])

            # Ensure positions are within bounds
            x = max(0, min(pixelated_width - 1, x))
            y = max(0, min(pixelated_height - 1, y))

            positions[index] = (x, y)
    else:
        positions[:] = original_positions[:]

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Enhance the contrast of the frame
    frame = enhance_contrast(frame)

    # Convert frame to grayscale for motion detection
    gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray_frame = cv2.GaussianBlur(gray_frame, (21, 21), 0)

    # Compute the absolute difference between the current frame and the previous frame
    frame_diff = cv2.absdiff(prev_gray, gray_frame)
    threshold_frame = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)[1]
    threshold_frame = cv2.dilate(threshold_frame, None, iterations=2)
    
    # Check if there is any motion detected
    motion_detected = np.sum(threshold_frame) > 10000

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
