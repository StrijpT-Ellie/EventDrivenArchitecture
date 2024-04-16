#This script creates a particle
#It is possible to switch to hand control 
#It is possible to trigger the emitter and get more particles 
#the main particle has acceleration and damping
#the particles have random initial acceleration
#the particles have random initial direction
#the particles have random color
#the particles have random speed
#edges are detected and the particles bounce off the edges

#add collision 
#add snake game mechanics 
#add scoring 
#add collision between main particles and emitted particles


import threading
import pygame
import random
import cv2
import numpy as np
import mediapipe as mp
import cv2
import math
import matplotlib.pyplot as plt

def visualize_array(array):
    plt.imshow(array, cmap='gray', interpolation='nearest')
    plt.show()

class HandController:
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        self.mp_hands = mp.solutions.hands
        self.hands = self.mp_hands.Hands()
        self.mp_draw = mp.solutions.drawing_utils
        self.old_hand_landmarks = None

    def get_direction(self):
        direction = 'none'  
        ret, frame = self.cap.read()
        frame = cv2.flip(frame, 1)
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = self.hands.process(frame_rgb)

        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                self.mp_draw.draw_landmarks(frame, hand_landmarks, self.mp_hands.HAND_CONNECTIONS)

            movement_threshold = 0.01  # Adjust this value to change the sensitivity

            if self.old_hand_landmarks is not None:
                dx = hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].x - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].x
                dy = hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].y - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].y
                if abs(dx) > movement_threshold or abs(dy) > movement_threshold:
                    if abs(dx) > abs(dy):
                        direction = 'right' if dx > 0 else 'left'
                    else:
                        direction = 'down' if dy > 0 else 'up'
                    #print(f"Hand movement: dx={dx}, dy={dy}, direction={direction}")  # Debug print

            self.old_hand_landmarks = hand_landmarks  # Update old_hand_landmarks

        return direction

    def release(self):
        self.cap.release()
        self.hands.close()
        

class Particle:
    def __init__(self, x, y, speed, direction, color):
        self.x = x
        self.y = y
        self.speed = speed 
        self.color = color 
        self.direction = direction
        self.dx = random.choice([-speed, speed])
        self.dy = random.choice([-speed, speed])
        self.ax = random.uniform(-0.1, 0.1)  # Add random initial acceleration
        self.ay = random.uniform(-0.1, 0.1)  # Add random initial acceleration
        self.damping = 0.99  # Adjust this value to change the rate of damping

    def move(self, width, height):
        self.dx += self.ax  # update velocity with acceleration
        self.dy += self.ay  # update velocity with acceleration
        self.dx *= self.damping  # apply damping to velocity
        self.dy *= self.damping  # apply damping to velocity

        # Predict next position
        next_x = self.x + self.dx
        next_y = self.y + self.dy

        # Check for collision with edges and reverse velocity if necessary
        if next_x < 0 or next_x > width:
            self.dx *= -1
        if next_y < 0 or next_y > height:
            self.dy *= -1

        # Update position
        self.x += self.dx
        self.y += self.dy

    def update_params(self, speed, direction):
        self.speed = speed
        self.direction = direction
        self.dx = speed * np.cos(np.radians(direction))
        self.dy = speed * np.sin(np.radians(direction))

    def update_acceleration(self, ax, ay):
        self.ax = ax * self.damping
        self.ay = ay * self.damping
        
class ParticleEmitter:
    def __init__(self, screen_width, screen_height, num_particles):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.num_particles = num_particles
        self.particles = []

    def generate_particle(self):
        x = self.screen_width
        y = random.randint(0, self.screen_height)
        speed = random.randint(1, 5)
        direction = random.randint(0, 360)
        color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
        return Particle(x, y, speed, direction, color)

    def trigger(self):
        for _ in range(self.num_particles):
            self.particles.append(self.generate_particle())
 
    def update(self):
        for particle in self.particles:
            particle.move(self.screen_width, self.screen_height)
            if particle.x < 0:
                self.particles.remove(particle)

    def draw(self, screen):
        for particle in self.particles:
            pygame.draw.circle(screen, particle.color, (particle.x, particle.y), 5)

class Block:
    def __init__(self, x, y, width, height, color):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.color = color

    def draw(self, screen):
        pygame.draw.rect(screen, self.color, pygame.Rect(self.x, self.y, self.width, self.height))

class BlockEmitter:
    def __init__(self, screen_width, screen_height, block_size):
        self.blocks = []
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.block_size = block_size

    def trigger(self):
        x = random.randint(0, self.screen_width - self.block_size)
        y = random.randint(0, self.screen_height - self.block_size)
        color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
        block = Block(x, y, self.block_size, self.block_size, color)
        self.blocks.append(block)

    def update(self, particles):
        for block in self.blocks[:]:
            for particle in particles:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    self.blocks.remove(block)
                    break

    def draw(self, screen):
        for block in self.blocks:
            block.draw(screen)


     
class Game:
    def __init__(self, width, height, grid_size, direction, num_particles, speed):
        pygame.init()  # Initialize all Pygame modules
        self.screen = pygame.display.set_mode((width, height))
        self.mode = 'autopilot'  # 'autopilot' or 'manual'
        self.hand_controller = HandController()

        # Define the width and height attributes
        self.width = width
        self.height = height

        # Define the particles attribute
        self.particles = [Particle(i % grid_size * (width // grid_size), 
                           i // grid_size * (height // grid_size), 
                           speed,
                           direction,  # Add direction
                           (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)))  # color
                  for i in range(num_particles)]
        
        self.output_arrays = []
        
    def run(self):
        
        running = True
        emitter = None
        block_emitter = BlockEmitter(self.width, self.height, 50)
        frame_counter = 0
        N = 32

        # Set print options
        np.set_printoptions(threshold=np.inf, linewidth=np.inf)
        
        while running:
            # Fill the screen with black
            self.screen.fill((0, 0, 0))

            # Draw the particles
            for particle in self.particles:
                pygame.draw.circle(self.screen, (255, 255, 255), (int(particle.x), int(particle.y)), 50)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_SPACE:  # Press space to switch mode
                        self.mode = 'manual' if self.mode == 'autopilot' else 'autopilot'
                    elif event.key == pygame.K_p:  # Press "p" to trigger the emitter
                        emitter = ParticleEmitter(self.width, self.height, 25)
                        emitter.trigger()
                    elif event.key == pygame.K_b:  # Press "b" to trigger the block emitter
                        block_emitter.trigger()
                   

            if self.mode == 'autopilot':
                for particle in self.particles:
                    particle.update_acceleration(random.uniform(-0.01, 0.01), random.uniform(-0.01, 0.01))  # Add random acceleration
                    particle.move(self.width, self.height)
            elif self.mode == 'manual':
                direction = self.hand_controller.get_direction()
                #print(f"Hand direction: {direction}")  # Debug print
                move_factor = 50  # Adjust this value to change the movement distance
                for particle in self.particles:
                    if direction == 'up':
                        particle.update_acceleration(0.5, -0.5)  # Increase acceleration upwards
                        particle.y = max(0, particle.y - particle.speed * move_factor)
                    elif direction == 'down':
                        particle.update_acceleration(0.5, 0.5)  # Increase acceleration downwards
                        particle.y = min(self.height, particle.y + particle.speed * move_factor)
                    elif direction == 'left':
                        particle.update_acceleration(-0.5, 0.5)  # Increase acceleration to the left
                        particle.x = max(0, particle.x - particle.speed * move_factor)
                    elif direction == 'right':
                        particle.update_acceleration(0.5, 0.5)  # Increase acceleration to the right
                        particle.x = min(self.width, particle.x + particle.speed * move_factor)
                    particle.move(self.width, self.height)  # Move the particle with the updated acceleration
                    particle.update_acceleration(0, 0)  # Reset acceleration to zero
                

                # Update and draw the blocks
                block_emitter.update(self.particles)
                block_emitter.draw(self.screen)

                if emitter:  # If the emitter has been triggered
                    emitter.update()
                    emitter.draw(self.screen)

           

        # Get the pixel values of the screen as a numpy array
            screen_array = pygame.surfarray.array3d(self.screen)

            # Resize the array to 20x20
            resized_array = cv2.resize(screen_array, (20, 20))

            # Reduce the color depth to 8-bit
            reduced_color_array = (resized_array / 32).astype(np.uint8) * 32

            # Store the array in self.output_arrays
            self.output_arrays.append(reduced_color_array)

            # Set print options
            #np.set_printoptions(threshold=np.inf)

            # Print the array to the terminal
            print(reduced_color_array)
            #visualize_array(reduced_color_array)

            pygame.display.flip()

            frame_counter += 1

             # Only visualize the array every N frames
            if frame_counter % N == 0:
                # Start a new thread to run the visualize_array function
                threading.Thread(target=visualize_array, args=(reduced_color_array,)).start()


        self.hand_controller.release()
        pygame.quit()

    #def get_output_arrays(self):
    # Print the first 5 output arrays
        #for array in self.output_arrays[:50]:
            #print(array)

if __name__ == "__main__":
    game = Game(600, 600, 20, 1, 1, 0.1)
    game.run()
    game.get_output_arrays()
