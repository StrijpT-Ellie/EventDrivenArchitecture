import csv
from ultralytics import YOLO

model = YOLO('/Users/krasnomakov/Documents1/py/ellie/yolov9-main/gelan-c.pt')
model.names

print(model.names)

# Save as CSV
with open('model_names.csv', 'w') as f:
    writer = csv.writer(f)
    for key, value in model.names.items():
        writer.writerow([key, value])