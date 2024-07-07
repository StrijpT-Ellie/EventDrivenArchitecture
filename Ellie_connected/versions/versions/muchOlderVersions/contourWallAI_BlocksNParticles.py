
import threading
import pygame
import random
import cv2
import numpy as np
import mediapipe as mp
import cv2
import math
import matplotlib.pyplot as plt
import time

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
    def __init__(self, screen_width, screen_height, num_particles, main_particle):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.num_particles = num_particles
        self.particles = []
        self.main_particle = main_particle  # Reference to the main particle

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

    def update(self, blocks):  # Accepts blocks parameter for collision detection
        for particle in self.particles:
            particle.move(self.screen_width, self.screen_height)

            # Check for collision with the main particle
            if self.check_collision(particle, self.main_particle):
                # Modify the small particle's movement based on the main particle's properties
                # Change the direction or speed of the small particle
                # Here, it just reversing the small particle's velocity
                particle.dx *= -5
                particle.dy *= -5

            for block in blocks:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    self.particles.remove(particle)
                    break

            if particle.x < 0:
                self.particles.remove(particle)

    def draw(self, screen):
        for particle in self.particles:
            pygame.draw.circle(screen, particle.color, (particle.x, particle.y), 20)

    def check_collision(self, particle1, particle2):
        # Check if two particles collide (simple collision detection based on distance)
        distance = math.sqrt((particle1.x - particle2.x)**2 + (particle1.y - particle2.y)**2)
        return distance < 12  # Modify this threshold as needed


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

    def trigger(self, num_blocks):
        for _ in range(num_blocks):
            x = random.randint(0, self.screen_width - self.block_size)
            y = random.randint(0, self.screen_height - self.block_size)
            color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            block = Block(x, y, self.block_size, self.block_size, color)
            self.blocks.append(block)

    def update(self, particles):  # Accepts particles parameter
        for block in self.blocks[:]:  # Iterate over a copy of blocks to allow removal
            for particle in particles:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    self.blocks.remove(block)
                    break

    def draw(self, screen):
        for block in self.blocks:
            block.draw(screen)




     
class Game:
    def __init__(self, width, height, grid_size, direction, num_particles, speed):
        pygame.init()
        self.screen = pygame.display.set_mode((width, height))
        self.mode = 'autopilot'
        self.hand_controller = HandController()
        self.width = width
        self.height = height

        # Frame rate control
        self.frame_rate = 30  # Adjust this value as needed
        self.last_print_time = time.time()
        self.print_interval = 1.0  # Adjust this value as needed

        # Create the main particle
        self.main_particle = Particle(width // 2, height // 2, speed, direction, (255, 255, 255))

        # Create the particle emitter with a reference to the main particle
        self.emitter = ParticleEmitter(self.width, self.height, 25, self.main_particle)


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
                pygame.draw.circle(self.screen, (255, 255, 255), (int(particle.x), int(particle.y)), 20)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_SPACE:  # Press space to switch modes
                        self.mode = 'manual' if self.mode == 'autopilot' else 'autopilot'

                    elif event.key == pygame.K_p:  # Press "p" to trigger the emitter
                        emitter = ParticleEmitter(self.width, self.height, 25, self.particles[0])  # Assuming the main particle is at index 0
                        emitter.trigger()


                    elif event.key == pygame.K_b:  # Press "b" to trigger the block emitter
                        block_emitter.trigger(25)  # Emit 25 blocks

                   

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
                    emitter.update(block_emitter.blocks)  # Pass the list of blocks to the particle emitter update method
                    emitter.draw(self.screen)

                # Update particles with blocks
                self.emitter.update(block_emitter.blocks)  # Pass the list of blocks to the particle emitter update method

           

        # Get the pixel values of the screen as a numpy array
            screen_array = pygame.surfarray.array3d(self.screen)

            # Resize the array to 20x20
            resized_array = cv2.resize(screen_array, (20, 20))

            # Reduce the color depth to 8-bit
            reduced_color_array = (resized_array / 32).astype(np.uint8) * 32

            # Store the array in self.output_arrays
            self.output_arrays.append(reduced_color_array)
            # Print the array to the terminal at intervals
            if time.time() - self.last_print_time >= self.print_interval:
                print(reduced_color_array)
                self.last_print_time = time.time()

            # Set frame rate control
            pygame.time.Clock().tick(self.frame_rate)

            pygame.display.flip()

            frame_counter += 1

            # Only visualize the array every N frames
            if frame_counter % N == 0:
                threading.Thread(target=visualize_array, args=(reduced_color_array,)).start()


        self.hand_controller.release()
        pygame.quit()

    #def get_output_arrays(self):
    # Print the first 5 output arrays
        #for array in self.output_arrays[:50]:
            #print(array)

if __name__ == "__main__":
    game = Game(600, 600, 20, 1, 50, 0.1)
    game.run()
    game.get_output_arrays()