#This code converts the camera feed to a 20 by 20 grid of "pixels"

import cv2
import numpy as np

# Define the dimensions for the pixelated image
pixelated_width, pixelated_height = 20, 20

# Define the dimensions for the enlarged pixelated display
display_width, display_height = 400, 400

# Open the camera
cap = cv2.VideoCapture(0)

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (pixelated_width, pixelated_height), interpolation=cv2.INTER_LINEAR)

    # Resize the pixelated image back to a larger size for display
    enlarged_pixelated = cv2.resize(pixelated, (display_width, display_height), interpolation=cv2.INTER_NEAREST)

    # Convert the pixelated image to a NumPy array
    pixelated_np = np.array(pixelated)
    
    # Print the final 20x20 pixelated array
    print(pixelated_np)

    # Display the enlarged pixelated image
    cv2.imshow('Pixelated', enlarged_pixelated)

    # Break the loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# After the loop release the cap object
cap.release()
# Destroy all the windows
cv2.destroyAllWindows()
