import sounddevice as sd
import numpy as np

def audio_callback(indata, frames, time, status):
    volume_norm = np.linalg.norm(indata) * 10
    print("|" * int(volume_norm))  # prints a bar of length proportional to volume

with sd.InputStream(callback=audio_callback):
    sd.sleep(10000)  # listens for 10 seconds