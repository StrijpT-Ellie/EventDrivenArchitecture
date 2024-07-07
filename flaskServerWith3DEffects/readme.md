This is an implementation of a simple AI event-driven architecture with Flask Server 

Demo: https://vimeo.com/929837304/6112b0460f?share=copy 

It uses yolov9 (gelan) model 
When the model detects a cell-phone or a bottle the listening script will send a POST request to the serer
Video output to pixelated array changes pixel size when sees a cell-phone, applies waves effect when sees a bottle

To run 

1. $ cd yolov9-main 
2. $ python3.11 detect.py --weights gelan-c.pt --conf 0.5 --source 0 --device cpu &> ../output.txt
3. $ cd ..
4. $ python3.11 eventTrigArch2.py (run in a dedicated terminal)
5. $ python3.11 app.py (run in a dedicated terminal)
6. optionally launch simulation.py to project JS output in the browser onto 20by20 pixelated array (imitation of LED PCB output)

