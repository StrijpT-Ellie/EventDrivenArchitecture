#This is a Flask server that renders a floating cube with some particles and a sphere using threeJS
#The server listens for POST requests to change the animation style and create a grid of cubes
#POST requests are sent from eventTrigArch2.py and running yolov9 on a video stream
#It reacts when either a cell phone or a bottle were detected 

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
# Global variable to store the last event type
last_event = None

@app.route('/change_animation', methods=['POST'])
def change_animation():
    global last_event
    print("Received POST request to /change_animation")
    new_animation_style = request.form.get('animation_style')
    last_event = new_animation_style
    return 'OK', 200

@app.route('/create_grid', methods=['POST'])
def create_grid():
    global last_event
    print("Received POST request to /create_grid")
    last_event = 'createGrid'
    return 'OK', 200

@app.route('/stream')
def stream():
    def event_stream():
        global last_event
        while True:
            if last_event:
                yield f'data: {last_event}\n\n'
                last_event = None
            
    return Response(event_stream(), mimetype='text/event-stream')

if __name__ == '__main__':
    app.run(debug=True)
