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
    files = [f for f in os.listdir(font_dir) if f[-4:] == ".csv"]
    for file in files:
        file_path = os.path.join(font_dir, file)
        data = open(file_path, "r").read()
        data = [[[255., 255., 255.] if c == '1' else [0., 0., 0.] for c in line.split(',')] for line in data.split("\n")]
        character_index[chr(int(file[5:-4]))] = (np.array(data), len(data[0]), len(data))
    return character_index

# Define the function to put text with typewriter effect
def put_text(frame, text: str, start, character_index):
    column_start = start[1]
    cursor = start
    for c in text:
        if c == "\n":
            cursor = [cursor[0]+6, column_start]
            continue
        letter = character_index[c]
        frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter[0]
        cursor[1] += letter[1]+1

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
    mat = np.zeros((output_window_size[0]*6, output_window_size[1]*12, 3), dtype=np.uint8)
    
    # Initialize the position for the next character
    next_char_position = [0, 0]

    # Display the image with typewriter effect
    for char in response:
        # Check if adding the next character exceeds the width of the image
        if next_char_position[1] + 12 > output_window_size[1] * 12:  # Adjust the spacing as needed
            next_char_position[0] += 6  # Adjust the line spacing as needed
            next_char_position[1] = 0
        
        # Put the character on the image
        put_text(mat, char, next_char_position, character_index)

        # Update the position for the next character
        next_char_position[1] += 12  # Adjust the vertical spacing as needed
        
        # Display the updated image
        cv.imshow("text", mat)
        cv.waitKey(100)  # Adjust the delay time as needed for typing speed

    # Wait for a few seconds after the text is fully displayed
    cv.waitKey(2000)
