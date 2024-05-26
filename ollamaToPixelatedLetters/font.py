import numpy as np
import os

character_index = {}

def load_character_index():
    dir_path = os.path.dirname(os.path.abspath(__file__))
    font_dir = os.path.join(dir_path, "font")
    files = [f for f in os.listdir(font_dir) if f[-4:] == ".csv"]
    for file in files:
        file_path = os.path.join(font_dir, file)
        data = open(file_path, "r").read()
        data = [[[255., 255., 255.] if c == '1' else [0., 0., 0.] for c in line.split(',')] for line in data.split("\n")]
        character_index[chr(int(file[5:-4]))] = (np.array(data), len(data[0]), len(data))

def put_text(frame, text: str, start):
    column_start = start[1]
    cursor = start
    for c in text:
        if c == "\n":
            cursor = [cursor[0]+6, column_start]
            continue
        letter = character_index[c]
        frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter[0]
        cursor[1] += letter[1]+1
