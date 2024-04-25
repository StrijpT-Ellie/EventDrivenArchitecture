import cv2 as cv
import numpy as np
from langchain_community.chat_models import ChatOllama
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
from font import put_text, load_character_index

# Load the character index
load_character_index()

# Initialize the ChatOllama model
llm = ChatOllama(model="gemma:2b", max_tokens=5, max_predict=5, temperature=0.5)
prompt = ChatPromptTemplate.from_template("Tell me a short joke about {topic}")
chain = prompt | llm | StrOutputParser()

while True:
    # Initialize the image
    mat = np.zeros((40, 250, 3))

    # Get the topic from the user
    topic = input("Please enter a topic: ")

    # Get the response from the model
    response = chain.invoke({"topic": topic})

    # Put the response text into the image
    put_text(mat, response, [1, 0])

    # Display the image
    cv.namedWindow('text', cv.WINDOW_NORMAL)
    cv.resizeWindow('text', 1200, 150)
    cv.imshow("text", mat)
    cv.waitKey(7000)  # Wait for 10 seconds
    cv.destroyAllWindows() 