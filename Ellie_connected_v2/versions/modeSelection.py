#This is a script for mode selection 
#It displays the modes available to the user
#The user can select a mode by showing fingers to the camera
#Modes are displayed as pixelated text
#When a number of fingers is shown for 6 seconds the corresponding mode is highlighted in green

import cv2 as cv
import numpy as np
import os

class CharacterLoader:
    def __init__(self):
        self.character_index = {}
        self.load_character_index()

    def load_character_index(self):
        dir_path = os.path.dirname(os.path.abspath(__file__))
        font_dir = os.path.join(dir_path, "font")
        files = [f for f in os.listdir(font_dir) if f[-4:] == ".csv"]
        for file in files:
            file_path = os.path.join(font_dir, file)
            data = open(file_path, "r").read()
            data = [[[255., 255., 255.] if c == '1' else [0., 0., 0.] for c in line.split(',')] for line in data.split("\n")]
            self.character_index[chr(int(file[5:-4]))] = (np.array(data), len(data[0]), len(data))

    def get_character(self, character):
        return self.character_index.get(character, None)

class TextWriter:
    def __init__(self, character_loader):
        self.character_loader = character_loader

    def put_text(self, frame, text, start):
        column_start = start[1]
        cursor = start
        char_count = 0
        for c in text:
            if c == "\n" or char_count == 50:
                cursor = [cursor[0]+6, column_start]
                char_count = 0
                if c == "\n":
                    continue
            letter = self.character_loader.get_character(c)
            if letter:
                frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter[0]
                cursor[1] += letter[1]+1
                char_count += 1

class TextDisplay:
    def __init__(self, text_writer):
        self.text_writer = text_writer

    def display_text(self, text):
        mat = np.zeros((40, 250, 3))
        self.text_writer.put_text(mat, text, [1, 0])
        cv.namedWindow('text', cv.WINDOW_NORMAL)
        cv.resizeWindow('text', 1200, 600)
        cv.imshow("text", mat)
        cv.waitKey(5000)
        cv.destroyAllWindows()

def main():
    loader = CharacterLoader()
    writer = TextWriter(loader)
    display = TextDisplay(writer)
    
    # The text to be displayed
    modes_text = "1 - art\n2 - game\n3 - chat"
    display.display_text(modes_text)

if __name__ == "__main__":
    main()
