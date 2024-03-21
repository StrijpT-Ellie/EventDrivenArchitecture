import cv2
import numpy as np

def generate_rainbow_frame(width, height):
    # Define rainbow colors
    rainbow_colors = [
        (255, 0, 0),     # Red
        (255, 165, 0),   # Orange
        (255, 255, 0),   # Yellow
        (0, 255, 0),     # Green
        (0, 127, 255),   # Blue
        (75, 0, 130),    # Indigo
        (148, 0, 211)    # Violet
    ]

    # Create an initial frame
    frame = np.zeros((height, width, 3), dtype=np.uint8)

    # Generate rainbow pattern
    for i in range(height):
        color = rainbow_colors[i % len(rainbow_colors)]
        frame[i, :] = color

    return frame

# Generate initial rainbow frame
frame = generate_rainbow_frame(400, 600)

# Create a window
cv2.namedWindow('Rainbow Frame', cv2.WINDOW_NORMAL)

# Animate the rainbow as a waterfall
i = 0
rainbow_colors = [
    (255, 0, 0),     # Red
    (255, 165, 0),   # Orange
    (255, 255, 0),   # Yellow
    (0, 255, 0),     # Green
    (0, 127, 255),   # Blue
    (75, 0, 130),    # Indigo
    (148, 0, 211)    # Violet
]

while True:
    # Shift the rows down
    frame[:-1] = frame[1:]

    # Insert a new row at the top
    frame[-1] = rainbow_colors[i % len(rainbow_colors)]

    # Display the frame
    cv2.imshow('Rainbow Frame', frame)

    # Break the loop if 'q' is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    # Move to the next color
    i += 1

cv2.destroyAllWindows()