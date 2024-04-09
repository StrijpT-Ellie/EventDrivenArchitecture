import torch
import torch.nn as nn
import matplotlib.pyplot as plt


# Define the neural network
class Net(nn.Module):
    def __init__(self, input_size):
        super(Net, self).__init__()
        self.fc1 = nn.Linear(input_size, 256)  # Fully connected layer
        self.fc2 = nn.Linear(256, 400)  # Output size is 20*20

    def forward(self, x):
        x = torch.relu(self.fc1(x))  # Activation function
        x = self.fc2(x)
        return x.view(-1, 20, 20)  # Reshape to 20x20

# Initialize the neural network
net = Net(input_size=100)

# Generate some random input
input_data = torch.randn(1, 100)

# Pass the input through the neural network
output = net(input_data)

# The output is a 20x20 array
print(output)

# Convert the tensor to numpy array for visualization
output_np = output.detach().numpy()

# Visualize the output
plt.imshow(output_np[0], cmap='gray')
plt.colorbar()
plt.show()