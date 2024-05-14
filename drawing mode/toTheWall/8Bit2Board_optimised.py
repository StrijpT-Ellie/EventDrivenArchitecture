import cv2
import numpy as np
import random
import sys
from contourwall import ContourWall

# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Define the lower and upper bounds for the color to track (in HSV space)
lower_color = np.array([100, 150, 150])  # Adjust these values for your specific color
upper_color = np.array([140, 255, 255])  # Adjust these values for your specific color

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

# Function to send pixel data to ContourWall
def send_to_contour_wall(cw, pixelated):
    for i in range(pixelated_height):
        for j in range(pixelated_width):
            color = pixelated[i, j]
            # Assuming cw.pixels is a 2D array-like structure where colors can be assigned directly
            cw.pixels[i, j] = color
    cw.show()

# Function to create the waterfall effect
def create_waterfall_effect(long_exposure_frame, pixelated):
    long_exposure_frame = np.roll(long_exposure_frame, 1, axis=0)
    for i in range(pixelated_height):
        for j in range(pixelated_width):
            long_exposure_frame[i, j] = pixelated[i, j]
    return long_exposure_frame

if __name__ == "__main__":
    cw = ContourWall()
    cw.single_new_with_port("COM4")

    # Initialize the long exposure frame
    long_exposure_frame = np.zeros((pixelated_height, pixelated_width, 3), dtype=np.uint8)

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

        # Apply the floating effect
        apply_floating_effect(pixel_positions)

        # Create the waterfall effect
        long_exposure_frame = create_waterfall_effect(long_exposure_frame, pixelated)

        # Send the pixelated data to ContourWall
        send_to_contour_wall(cw, long_exposure_frame)

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
