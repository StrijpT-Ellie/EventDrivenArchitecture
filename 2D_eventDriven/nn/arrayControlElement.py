import numpy as np
import matplotlib.pyplot as plt

# Create a 20x20 array with random values between 0 and 255
image = np.random.randint(0, 256, (20, 20))

# Apply a "visual effect" by multiplying each pixel by a factor
factor = 0.5
image = image * factor

# Ensure all values are integers between 0 and 255
image = np.clip(image, 0, 255).astype(int)

# Create a figure and axis to display the image
fig, ax = plt.subplots()

# Initial position of the "floating" pixel
x, y = 15, 5

# Loop to create a "vibrating" effect at pixel (10, 10) and a "floating" effect at pixel (x, y)
for _ in range(50):
    # Randomly change the value of the pixel at (10, 10)
    image[10, 10] = np.random.randint(0, 256)

    # Randomly change the value of the pixel at (x, y)
    image[x, y] = np.random.randint(0, 256)
    
    # Update the image display
    ax.imshow(image, cmap='gray')
    plt.pause(0.1)  # Pause for 0.1 seconds

    # Reset the pixel at (x, y) to its original value
    image[x, y] = factor * 255

    # Move the pixel to a new random position
    x = (x + np.random.randint(-1, 2)) % 20
    y = (y + np.random.randint(-1, 2)) % 20

plt.show()