from flask import Flask, render_template, request

app = Flask(__name__)

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        # Get the input values from the form
        num1 = request.form.get('num1', '')
        num2 = request.form.get('num2', '')
        num3 = request.form.get('num3', '')
        num4 = request.form.get('num4', '')
        num5 = request.form.get('num5', '')
        num6 = request.form.get('num6', '')
        num7 = request.form.get('num7', '')
        num8 = request.form.get('num8', '')
        num9 = request.form.get('num9', '')
        num10 = request.form.get('num10', '')

        # Call the post_function with the input values
        post_function(num1, num2, num3, num4, num5, num6, num7, num8, num9, num10)

        # Render the template with the input values
        return render_template('index.html', num1=num1, num2=num2, num3=num3, num4=num4, num5=num5,
                               num6=num6, num7=num7, num8=num8, num9=num9, num10=num10)

    # Render the template for GET requests
    return render_template('index.html')

def post_function(num1, num2, num3, num4, num5, num6, num7, num8, num9, num10):
    # Do something with the input values
    print(f"Received numbers: {num1}, {num2}, {num3}, {num4}, {num5}, {num6}, {num7}, {num8}, {num9}, {num10}")

if __name__ == '__main__':
    app.run(debug=True)
