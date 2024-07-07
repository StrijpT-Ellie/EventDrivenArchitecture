#this script returns letters with the proper size 
#however, they go in the diagonal and overlap after repeating 

import cv2 as cv
import numpy as np
import os
from langchain_community.chat_models import ChatOllama
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate

# Define the function to load the character index
def load_character_index():
    character_index = {}
    dir_path = os.path.dirname(os.path.abspath(__file__))
    font_dir = os.path.join(dir_path, "font")
    files = [f for f in os.listdir(font_dir) if f.endswith(".csv")]
    for file in files:
        file_path = os.path.join(font_dir, file)
        data = open(file_path, "r").read()
        data = [[[255., 255., 255.] if c == '1' else [0., 0., 0.] for c in line.split(',')] for line in data.split("\n")]
        character_index[chr(int(file[5:-4]))] = (np.array(data), len(data[0]), len(data))
    return character_index

def put_text(frame, text: str, start, character_index, scale=10):
    row_start, column_start = start
    row_spacing = 12 * scale  # Adjust as needed
    column_spacing = 8 * scale  # Adjust as needed
    cursor_row = row_start
    cursor_column = column_start
    max_width = 0
    max_height = 0
    for c in text:
        if c == "\n":
            cursor_row += row_spacing  # Move to the next row
            cursor_column = column_start  # Reset column position for new line
            continue
        if c not in character_index:
            # Skip characters not found in the character index
            continue
        letter_data, letter_width, letter_height = character_index[c]

        # Scale up the size of the letter
        scaled_letter_width = letter_width * scale
        scaled_letter_height = letter_height * scale

        # Calculate the maximum width and height of the text
        max_width = max(max_width, cursor_column + scaled_letter_width)
        max_height = max(max_height, cursor_row + scaled_letter_height)

        # Check if adding the letter exceeds the frame boundaries
        if cursor_row + scaled_letter_height > frame.shape[0] or cursor_column + scaled_letter_width > frame.shape[1]:
            # If adding the letter exceeds the frame boundaries, stop adding text
            break

        # Place each pixel of the letter onto the frame
        for i in range(scaled_letter_height):
            for j in range(scaled_letter_width):
                frame[cursor_row + i][cursor_column + j] = letter_data[i // scale][j // scale]

        # Update cursor position for the next character (move right)
        cursor_column += scaled_letter_width + column_spacing

    return [max_height, max_width]  # Return updated cursor position

# Load the character index
character_index = load_character_index()

# Initialize the ChatOllama model
llm = ChatOllama(model="ICTjokes", stream=True)
prompt = ChatPromptTemplate.from_template("Tell me a short joke about {topic}. Do not start with Sure here is the joke... or similar.")
chain = prompt | llm | StrOutputParser()

# Define the topic dictionary
topic_dict = {
    1: "Software",
    2: "Hardware",
    3: "Infrastructure",
    4: "Media Design",
    5: "Business"
}

# Set the size of the output window
output_window_size = (20, 20)

while True:
    # Get the topic from the user
    topic_num = int(input("Please enter a number (1-Software, 2-Hardware, 3-Infrastructure, 4-Media Design, 5-Business): "))

    # Check if the input number is valid
    if topic_num not in topic_dict:
        print("Invalid number. Please enter a number between 1 and 5.")
        continue

    # Get the corresponding topic
    topic = topic_dict[topic_num]

    # Get the response from the model
    response = chain.invoke({"topic": topic})

    # Initialize the image
    mat = np.zeros((1200*12, 600*12, 3), dtype=np.uint8)  # Adjust the dimensions as needed
    
    # Initialize the position for the next character
    next_char_position = [0, 0]

    # Display the image with typewriter effect
    for char in response:
        # Check if adding the next character exceeds the width of the image
        if next_char_position[1] + 12 > 20 * 12:  # Adjust the spacing as needed
            next_char_position[0] += 12  # Adjust the line spacing as needed
            next_char_position[1] = 0  # Reset column position for new line
        # Put the character on the image and get the next character position
        next_char_position = put_text(mat, char, next_char_position, character_index)
        # Display the updated image
        cv.imshow("text", mat)
        cv.waitKey(100)  # Adjust the delay time as needed for typing speed


    # Wait for a few seconds after the text is fully displayed
    cv.waitKey(2000)
