import cv2
import numpy as np
from contourwall import ContourWall
import time

# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Open the camera
cap = cv2.VideoCapture(0)

# Initialize ContourWall
cw = ContourWall()
cw.single_new_with_port("COM4")

# Create an overlay for drawing
overlay = np.zeros((pixelated_height, pixelated_width, 3), dtype=np.uint8)

# Function to send the pixelated data to ContourWall
def send_to_contour_wall(cw, pixelated):
    for i in range(pixelated_height):
        for j in range(pixelated_width):
            # Convert BGR to RGB for ContourWall
            color = pixelated[i, j][::-1]
            cw.pixels[i, j] = color
    cw.show()

# Track last frame to detect movement
ret, last_frame = cap.read()
last_frame = cv2.resize(last_frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)
last_frame_gray = cv2.cvtColor(last_frame, cv2.COLOR_BGR2GRAY)

# Parameters for drawing traces
trace_duration = 5  # seconds
trace_fade_time = time.time() + trace_duration

# Function to adjust contrast
def adjust_contrast(image, alpha=2.0, beta=0):
    """
    Adjust the contrast of an image.
    
    :param image: Input image
    :param alpha: Contrast control (1.0-3.0)
    :param beta: Brightness control (0-100)
    :return: Contrast adjusted image
    """
    return cv2.convertScaleAbs(image, alpha=alpha, beta=beta)

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Adjust the contrast of the pixelated image
    pixelated = adjust_contrast(pixelated, alpha=2.0, beta=-1)

    # Convert the pixelated image to a NumPy array
    pixelated_np = np.array(pixelated)

    # Convert current frame to grayscale for movement detection
    current_frame_gray = cv2.cvtColor(pixelated, cv2.COLOR_BGR2GRAY)

    # Calculate absolute difference between the current frame and the last frame
    frame_diff = cv2.absdiff(current_frame_gray, last_frame_gray)

    # Threshold the difference to get the areas with significant changes
    _, thresh = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)

    # Find contours of the thresholded areas
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # Draw the contours on the overlay with the desired color (red or yellow)
    for contour in contours:
        if cv2.contourArea(contour) > 50:  # Filter out small contours
            color = (0, 0, 255) if time.time() % 2 < 1 else (0, 255, 255)  # Red and Yellow
            cv2.drawContours(overlay, [contour], -1, color, -1)

    # Fade the overlay over time
    current_time = time.time()
    if current_time > trace_fade_time:
        overlay = cv2.addWeighted(overlay, 0.9, np.zeros_like(overlay), 0.1, 0)
        trace_fade_time = current_time + trace_duration

    # Update the last frame
    last_frame_gray = current_frame_gray

    # Combine the overlay with the current pixelated image
    combined = cv2.addWeighted(pixelated, 0.7, overlay, 0.3, 0)

    # Send the combined image to ContourWall
    send_to_contour_wall(cw, combined)

    # Display the combined image on screen
    cv2.imshow('Drawing Mode', combined)

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
