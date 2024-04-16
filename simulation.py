import requests
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

def get_pixel_data():
    response = requests.get('http://localhost:5000/pixels')
    pixel_array = np.array(response.json())
    return pixel_array

def update(i):
    pixel_array = get_pixel_data()
    im.set_array(pixel_array)
    return im,

if __name__ == '__main__':
    fig, ax = plt.subplots()
    pixel_array = get_pixel_data()
    im = ax.imshow(pixel_array)

    ani = FuncAnimation(fig, update, frames=range(100), interval=5, blit=True)

    plt.show()