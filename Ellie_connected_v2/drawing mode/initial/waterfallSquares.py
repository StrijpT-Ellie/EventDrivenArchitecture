#Projects a video stream to pixelated grid
#Imitates waterfall effect with squares

import cv2
import numpy as np

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

# Previous position of the object
prev_x, prev_y = None, None

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

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

        # Draw on the canvas if the object is detected
        if radius > 5:
            if prev_x is not None and prev_y is not None:
                cv2.line(canvas, (prev_x, prev_y), (center[0], center[1]), (255, 0, 0), 5)
            prev_x, prev_y = center
        else:
            prev_x, prev_y = None, None
    else:
        prev_x, prev_y = None, None

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Resize the pixelated image back to a larger size for display
    enlarged_pixelated = cv2.resize(pixelated, (display_width, display_height), interpolation=cv2.INTER_NEAREST)

    # Initialize the canvas if it's None
    if canvas is None:
        canvas = np.zeros_like(enlarged_pixelated)

    # Add the current frame to the long exposure frame
    if long_exposure_frame is None:
        long_exposure_frame = np.zeros_like(enlarged_pixelated, dtype=np.float32)
    long_exposure_frame = cv2.addWeighted(long_exposure_frame, 0.95, enlarged_pixelated.astype(np.float32), 0.05, 0)

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
