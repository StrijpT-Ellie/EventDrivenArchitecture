import cv2
import numpy as np
import pyee
import tailer
import threading

# Define the dimensions for the pixelated image
width, height = 100, 100
new_width, new_height = 500, 500
original_width, original_height = width, height

# Create an event emitter
ee = pyee.EventEmitter()

# Define a flag for applying the wave effect
apply_wave = False

# Define a function that changes the pixel size
def change_pixel_size(new_width, new_height):
    global width, height
    width, height = new_width, new_height

# Define a function that resets the pixel size
def reset_pixel_size():
    global width, height
    width, height = original_width, original_height

# Define a function that enables the wave effect
def enable_wave_effect():
    global apply_wave
    apply_wave = True

# Define a function that disables the wave effect
def disable_wave_effect():
    global apply_wave
    apply_wave = False

# Add listeners to the event emitter
ee.on('change_pixel_size', change_pixel_size)
ee.on('reset_pixel_size', reset_pixel_size)
ee.on('enable_wave_effect', enable_wave_effect)
ee.on('disable_wave_effect', disable_wave_effect)

# Function to follow the file and emit events
import requests

# Function to follow the file and emit events
def follow_file_and_emit_events():
    with open('/Users/krasnomakov/Documents1/py/ellie/EventDrivenArchitecture/flaskServerWith3DEffects/output.txt', 'r') as f:
        for line in tailer.follow(f):
            if 'cell phone' in line:
                ee.emit('change_pixel_size', new_width, new_height)

                # Define the URL
                url = "http://localhost:5000/change_animation"

                # Define the data payload
                data = {"animation_style": "divide"}

                # Send a POST request
                response = requests.post(url, data=data)

                # Print the response
                print(response.text)

            elif 'bottle' in line:
                ee.emit('enable_wave_effect')
            else:
                ee.emit('reset_pixel_size')
                ee.emit('disable_wave_effect')

# Run the function in a separate thread
threading.Thread(target=follow_file_and_emit_events).start()

# Open the camera
cap = cv2.VideoCapture(0)

# Function to apply the wave effect
def apply_wave_effect(frame):
    # Get the image shape
    rows, cols = frame.shape[:2]

    # Create a meshgrid for the image
    img_x, img_y = np.meshgrid(np.arange(cols), np.arange(rows))

    # Create a wave effect by modifying the x and y coordinates
    img_x_new = img_x + 15*np.sin(2*np.pi*img_y/64)
    img_y_new = img_y + 15*np.sin(2*np.pi*img_x/64)

    # Remap the image
    waved = cv2.remap(frame, img_x_new.astype(np.float32), img_y_new.astype(np.float32), interpolation=cv2.INTER_LINEAR, borderMode=cv2.BORDER_REFLECT)

    return waved

# Set print options - uncomment to display complete array (very laggy)
#np.set_printoptions(threshold=np.inf)

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()

    # Resize the frame to the desired dimensions
    pixelated = cv2.resize(frame, (width, height), interpolation=cv2.INTER_LINEAR)

    # Resize the pixelated image back to the original size
    pixelated = cv2.resize(pixelated, (frame.shape[1], frame.shape[0]), interpolation=cv2.INTER_NEAREST)

    # Apply the wave effect if the flag is set
    if apply_wave:
        pixelated = apply_wave_effect(pixelated)
        
    # Resize the processed image back to 20x20 pixels
    pixelated = cv2.resize(pixelated, (400, 400), interpolation=cv2.INTER_LINEAR)  #Change the size here to manipulate the final image size

    # Convert the pixelated image to a NumPy array
    pixelated_np = np.array(pixelated)
    
    # Print the final output
    print(pixelated_np)

    # Display the resulting frame
    cv2.imshow('Pixelated', pixelated_np)

    # Break the loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break



# After the loop release the cap object
cap.release()
# Destroy all the windows
cv2.destroyAllWindows()