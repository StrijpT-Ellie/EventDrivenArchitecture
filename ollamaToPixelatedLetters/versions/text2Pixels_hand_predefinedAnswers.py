import cv2 as cv
import numpy as np
from langchain_community.chat_models import ChatOllama
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
from font import put_text, load_character_index

# Load the character index
load_character_index()

# Initialize the ChatOllama model
llm = ChatOllama(model="ICTjokes", stream = True)
prompt = ChatPromptTemplate.from_template("Tell me a few words about {topic}. Only 2-3 words.")
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
    # Initialize the image
    mat = np.zeros((40, 250, 3))

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

    print(response)

    # Put the response text into the image
    put_text(mat, response, [1, 0])

    # Display the image
    cv.namedWindow('text', cv.WINDOW_NORMAL)
    cv.resizeWindow('text', 1200, 150)
    cv.imshow("text", mat)
    cv.waitKey(7000)  # Wait for 10 seconds
    cv.destroyAllWindows() 