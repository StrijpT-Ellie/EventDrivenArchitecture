#This script converts the camera feed to a 20 by 20 grid of black and white pixels.
#It only returns black and white pixels, with no shades of gray.
#The video is indistinguishable
#It mainly reacts on light and dark areas in the input
#Close your camera with a hand to see the dark screen 
#Open the camera to see white moving pixels

import numpy as np
import matplotlib.pyplot as plt
import cv2

# Create a figure and axis to display the image
fig, ax = plt.subplots()

# Open the camera
cap = cv2.VideoCapture(0)

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
pixel_positions = [(j, i) for i in range(20) for j in range(20)]
pixel_velocities = [(0, 0) for _ in range(400)]

# Function to apply floating effect
def apply_floating_effect(positions, velocities):
    for index, ((x, y), (vx, vy)) in enumerate(zip(positions, velocities)):
        # Randomly change velocity if motion is detected
        if motion_detected:
            vx = np.random.randint(-1, 2)
            vy = np.random.randint(-1, 2)
        else:
            vx *= 0.95
            vy *= 0.95

        # Update positions
        new_x = (x + vx) % 20
        new_y = (y + vy) % 20
        positions[index] = (new_x, new_y)
        velocities[index] = (vx, vy)

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
    pixelated = cv2.resize(frame, (20, 20), interpolation=cv2.INTER_LINEAR)

    # Quantize the colors to primary RGB values
    pixelated = quantize_colors(pixelated)

    # Apply floating effect
    apply_floating_effect(pixel_positions, pixel_velocities)

    # Convert the pixelated image back to grayscale for matplotlib display
    grayscale_pixelated = np.zeros((20, 20), dtype=np.uint8)
    for idx, (x, y) in enumerate(pixel_positions):
        i, j = divmod(idx, 20)
        grayscale_pixelated[int(y), int(x)] = pixelated[i, j, 0]

    # Update the image display
    ax.imshow(grayscale_pixelated, cmap='gray')
    plt.pause(0.1)  # Pause for 0.1 seconds

    # Break the loop on 'q' key press
    if plt.waitforbuttonpress(0.1):
        break

# After the loop release the cap object
cap.release()
plt.close(fig)
