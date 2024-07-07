import cv2
import numpy as np
import random
from contourwall import ContourWall

# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Define the dimensions for the enlarged pixelated display
display_width, display_height = 400, 400

# Open the camera
cap = cv2.VideoCapture(0)

# Initialize ContourWall
cw = ContourWall()
cw.single_new_with_port("COM4")

# Initialize the long exposure frame
long_exposure_frame = np.zeros((pixelated_width, pixelated_height, 3), dtype=np.uint8)

# Initialize previous frame for motion detection
ret, prev_frame = cap.read()
prev_gray = cv2.cvtColor(prev_frame, cv2.COLOR_BGR2GRAY)
prev_gray = cv2.GaussianBlur(prev_gray, (21, 21), 0)

# Initialize pixel positions for floating effect
pixel_positions = [(j, i) for i in range(pixelated_height) for j in range(pixelated_width)]

def enhance_contrast(image):
    lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
    cl = clahe.apply(l)
    limg = cv2.merge((cl, a, b))
    final = cv2.cvtColor(limg, cv2.COLOR_LAB2BGR)
    return final

def send_to_contour_wall(cw, pixelated):
    for i in range(pixelated_height):
        for j in range(pixelated_width):
            color = pixelated[i, j]
            cw.pixels[i, j] = color
    cw.show()

def apply_floating_effect(positions):
    for index in range(len(positions)):
        x, y = positions[index]
        x += random.choice([-1, 0, 1])
        y += random.choice([-1, 0, 1])
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

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Apply the floating effect if motion is detected
    if motion_detected:
        apply_floating_effect(pixel_positions)

    # Create a blank frame for the floating effect
    floating_frame = np.zeros((pixelated_height, pixelated_width, 3), dtype=np.uint8)

    # Assign pixelated colors to the floating positions
    for (x, y) in pixel_positions:
        floating_frame[y, x] = pixelated[y, x]

    # Send the pixelated data to ContourWall
    send_to_contour_wall(cw, floating_frame)

    # Resize the floating frame for display
    enlarged_floating = cv2.resize(floating_frame, (display_width, display_height), interpolation=cv2.INTER_NEAREST)

    # Display the floating frame
    cv2.imshow('Floating Pixels', enlarged_floating)

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
