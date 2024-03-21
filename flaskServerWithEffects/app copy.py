#this backend is suitable for automatic drawing. 
#when event is called the code starts repeating it by itself
#it forces the cube to leave traces like a brush


import numpy as np
from flask import request, Response
from mss import mss
from flask import Flask, render_template
from flask_assets import Environment, Bundle

app = Flask(__name__)
assets = Environment(app)

js = Bundle('scriptThreeJS.js', filters='jsmin', output='gen/main.min.js')
assets.register('main_js', js)

@app.route('/')
def index():
    with mss() as sct:
        screenshot = sct.grab(sct.monitors[0])
        img_array = np.array(screenshot)
    return render_template('index.html')

received_post = False

@app.route('/change_animation', methods=['POST'])
def change_animation():
    global received_post
    print("Received POST request to /change_animation")
    new_animation_style = request.form.get('animation_style')
    received_post = True
    def message():
        yield f"data: {new_animation_style}\n\n"
    return Response(message(), mimetype='text/event-stream')


@app.route('/stream')
def stream():
    def event_stream():
        if received_post:
            yield 'data: divide\n\n'
    return Response(event_stream(), mimetype='text/event-stream')

if __name__ == '__main__':
    app.run(debug=True)
