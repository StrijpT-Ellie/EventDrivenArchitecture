import numpy as np
import matplotlib.pyplot as plt
import cv2
import random

# Create a 20x20 array with random values between 0 and 255
image = np.random.randint(0, 256, (20, 20), dtype=np.uint8)

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
        # Randomly change velocity
        vx = random.randint(-1, 1)
        vy = random.randint(-1, 1)
        # Update positions
        new_x = (x + vx) % 20
        new_y = (y + vy) % 20
        positions[index] = (new_x, new_y)
        velocities[index] = (vx, vy)

# Create a figure and axis to display the image
fig, ax = plt.subplots()

# Open the camera
cap = cv2.VideoCapture(0)

# Initialize a canvas for drawing
canvas = None

# Initialize the long exposure frame
long_exposure_frame = None

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    
    # Check if frame is captured
    if not ret:
        break

    # Enhance the contrast of the frame
    frame = enhance_contrast(frame)

    # Resize the frame to the desired dimensions (20x20) to pixelate it
    pixelated = cv2.resize(frame, (20, 20), interpolation=cv2.INTER_LINEAR)

    # Quantize the colors to primary RGB values
    pixelated = quantize_colors(pixelated)

    # Convert the pixelated image back to grayscale for matplotlib display
    grayscale_pixelated = cv2.cvtColor(pixelated, cv2.COLOR_BGR2GRAY)

    # Apply floating effect
    apply_floating_effect(pixel_positions, pixel_velocities)

    # Display the pixelated image with floating effect in matplotlib
    for idx, (x, y) in enumerate(pixel_positions):
        i, j = divmod(idx, 20)
        grayscale_pixelated[y, x] = np.random.randint(0, 256)

    # Update the image display
    ax.imshow(grayscale_pixelated, cmap='gray')
    plt.pause(0.1)  # Pause for 0.1 seconds

    # Convert the pixelated image to BGR for OpenCV display
    bgr_pixelated = cv2.cvtColor(grayscale_pixelated, cv2.COLOR_GRAY2BGR)

    # Initialize the canvas if it's None
    if canvas is None:
        canvas = np.zeros_like(bgr_pixelated)

    # Add the current frame to the long exposure frame
    if long_exposure_frame is None:
        long_exposure_frame = np.zeros_like(bgr_pixelated, dtype=np.float32)
    long_exposure_frame = cv2.addWeighted(long_exposure_frame, 0.95, bgr_pixelated.astype(np.float32), 0.05, 0)

    # Apply the waterfall effect by shifting the pixels downward
    long_exposure_frame = np.roll(long_exposure_frame, 1, axis=0)

    # Convert the long exposure frame to 8-bit
    long_exposure_frame_8bit = cv2.convertScaleAbs(long_exposure_frame)

    # Combine the canvas with the long exposure frame
    combined_frame = cv2.addWeighted(long_exposure_frame_8bit, 0.7, canvas, 0.3, 0)

    # Display the combined frame using OpenCV
    cv2.imshow('Pixelated with Floating Effect', combined_frame)

    # Break the loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# After the loop release the cap object
cap.release()
# Destroy all the windows
cv2.destroyAllWindows()

plt.show()
