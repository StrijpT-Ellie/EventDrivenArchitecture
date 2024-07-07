tu run 

sudo dbus-launch ./zoomedPong --network=resnet18-hand --topology=~/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json --input-width=1280 --input-height=900 --input-codec=mjpeg /dev/video0
