import json
import trt_pose.coco
import trt_pose.models
import torch
import torch2trt
import time
import cv2
import torchvision.transforms as transforms
import PIL.Image
from trt_pose.draw_objects import DrawObjects
from trt_pose.parse_objects import ParseObjects
from jetcam.usb_camera import USBCamera
from jetcam.utils import bgr8_to_jpeg

# Load human pose JSON
with open('human_pose.json', 'r') as f:
    human_pose = json.load(f)

topology = trt_pose.coco.coco_category_to_topology(human_pose)

# Load the model
num_parts = len(human_pose['keypoints'])
num_links = len(human_pose['skeleton'])
model = trt_pose.models.resnet18_baseline_att(num_parts, 2 * num_links).cuda().eval()

# Load the model weights
MODEL_WEIGHTS = 'resnet18_baseline_att_224x224_A_epoch_249.pth'
model.load_state_dict(torch.load(MODEL_WEIGHTS))

# Create example data
WIDTH = 224
HEIGHT = 224
data = torch.zeros((1, 3, HEIGHT, WIDTH)).cuda()

# Optimize the model using torch2trt
model_trt = torch2trt.torch2trt(model, [data], fp16_mode=True, max_workspace_size=1<<25)

# Save the optimized model
OPTIMIZED_MODEL = 'resnet18_baseline_att_224x224_A_epoch_249_trt.pth'
torch.save(model_trt.state_dict(), OPTIMIZED_MODEL)

# Load the optimized model
from torch2trt import TRTModule
model_trt = TRTModule()
model_trt.load_state_dict(torch.load(OPTIMIZED_MODEL))

# Benchmark the model
t0 = time.time()
torch.cuda.current_stream().synchronize()
for i in range(50):
    y = model_trt(data)
torch.cuda.current_stream().synchronize()
t1 = time.time()
print(50.0 / (t1 - t0))

# Preprocess the image
mean = torch.Tensor([0.485, 0.456, 0.406]).cuda()
std = torch.Tensor([0.229, 0.224, 0.225]).cuda()
device = torch.device('cuda')

def preprocess(image):
    global device
    device = torch.device('cuda')
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    image = PIL.Image.fromarray(image)
    image = transforms.functional.to_tensor(image).to(device)
    image.sub_(mean[:, None, None]).div_(std[:, None, None])
    return image[None, ...]

# Define callable classes to parse and draw objects
parse_objects = ParseObjects(topology)
draw_objects = DrawObjects(topology)

# Create the camera
camera = USBCamera(width=WIDTH, height=HEIGHT, capture_fps=30)
camera.running = True

# Define the main execution loop
def execute(change):
    image = change['new']
    data = preprocess(image)
    cmap, paf = model_trt(data)
    cmap, paf = cmap.detach().cpu(), paf.detach().cpu()
    counts, objects, peaks = parse_objects(cmap, paf)  # , cmap_threshold=0.15, link_threshold=0.15
    draw_objects(image, counts, objects, peaks)
    image_w.value = bgr8_to_jpeg(image[:, ::-1, :])

# Execute the function once on the current camera frame
execute({'new': camera.value})

# Attach the execution function to the camera's internal value
camera.observe(execute, names='value')

# Unattach the camera frame callbacks
camera.unobserve_all()
