#updated to text2Pixels_Handcontroller.py

import cv2 as cv
import numpy as np
from langchain_community.chat_models import ChatOllama
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
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
    char_count = 0  # Initialize character counter
    for c in text:
        if c == "\n" or char_count == 50:  # Check if character is newline or 15 characters have been processed
            cursor = [cursor[0]+6, column_start]
            char_count = 0  # Reset character counter
            if c == "\n":
                continue
        letter = character_index[c]
        frame[cursor[0]:cursor[0]+letter[2], cursor[1]:cursor[1]+letter[1]] = letter[0]
        cursor[1] += letter[1]+1
        char_count += 1  # Increment character counter


# Load the character index
load_character_index()

# Initialize the ChatOllama model
llm = ChatOllama(model="qwen:0.5b", stream=True)
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

    # Typewriter effect
    for i in range(len(response) + 1):
        # Display part of the response
        mat = np.zeros((40, 250, 3))  # Clear the image for each frame
        put_text(mat, response[:i], [1, 0])  # Display the response up to the i-th character
        
        # Display the image
        cv.namedWindow('text', cv.WINDOW_NORMAL)
        cv.resizeWindow('text', 1200, 600)
        cv.imshow("text", mat)
        cv.waitKey(200)  # Short delay between characters

    # Wait for some time after the full message is displayed
    cv.waitKey(5000)  # Wait for 5 seconds

    # Destroy all windows to clean up
    cv.destroyAllWindows()
