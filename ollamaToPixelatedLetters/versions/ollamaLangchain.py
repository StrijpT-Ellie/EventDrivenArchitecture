from langchain_community.chat_models import ChatOllama
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
from langchain_community.llms import ollama


# supports many more optional parameters. Hover on your `ChatOllama(...)`
# class to view the latest available supported parameters
llm = ChatOllama(model="gemma:2b", max_tokens=5, max_predict=5, temperature=0.5)
prompt = ChatPromptTemplate.from_template("Tell me a short joke about {topic}")

# using LangChain Expressive Language chain syntax
# learn more about the LCEL on
# /docs/expression_language/why
chain = prompt | llm | StrOutputParser()

while True:
    # Get the topic from the user
    topic = input("Please enter a topic: ")

    # for brevity, response is printed in terminal
    # You can use LangServe to deploy your application for
    # production
    print(chain.invoke({"topic": topic}))