#The script works as a brick pong game with hand controller 
#The particle collides with the bar and the blocks
#Exists a chance of vertical bouncing loop 
#Must add some random acceleration or direction change when collides with blocks or a bar 
#Particle movement is optimised and can be faster or slower and the speed decreases when it is at the edge or at the bottom

#Add more to block collision - make it smother and make the particle do not go through or too fast
#consider random block structures appear 
#maybe add falling blocks
#Modify particles from particle emitter collision with blocks and add collision with the bar


import threading
import pygame
import random
import cv2
import numpy as np
import mediapipe as mp
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

            movement_threshold = 0.005  # Adjust this value to change the sensitivity

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
        self.direction = math.radians(direction)
        self.dx = self.speed * math.cos(self.direction)
        self.dy = self.speed * math.sin(self.direction)
        #self.ax = 1.5
        #self.ay = 1.5
        self.ax = random.uniform(-1, 0)  # Add random initial acceleration
        self.ay = random.uniform(-1, 1)  # Add random initial acceleration
        self.damping = 0.98  # Reduce this to smooth out movement

    def move(self, width, height):
        # Apply acceleration to velocity
        self.dx += self.ax
        self.dy += self.ay
        
        # Apply damping to smooth out movement
        self.dx *= self.damping
        self.dy *= self.damping

        # Predict next position
        next_x = self.x + self.dx
        next_y = self.y + self.dy

        # Check for collision with edges and adjust velocity to avoid sharp bouncing
        if next_x < 0 or next_x > width:
            self.dx *= -1  # Lose some velocity on collision to reduce jitter
        if next_y < 0 or next_y > height:
            self.dy *= -1  # Lose some velocity on collision to reduce jitter

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
        speed = random.randint(1, 10)
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
    def __init__(self, screen_width, screen_height, block_size, blocks_per_row):
        self.blocks = []
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.block_size = block_size
        self.blocks_per_row = blocks_per_row

    def trigger(self, num_blocks, block_width, block_height):
        for i in range(num_blocks):
            row = i // self.blocks_per_row
            column = i % self.blocks_per_row
            x = column * block_width
            y = row * block_height
            color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            block = Block(x, y, block_width, block_height, color)
            self.blocks.append(block)


    def update(self, particles):  # Accepts particles parameter
        for block in self.blocks[:]:  # Iterate over a copy of blocks to allow removal
            for particle in particles:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    self.blocks.remove(block)
                    particle.dy *= -1  # Reverse the y-component of the velocity
                    particle.dx *= -2
                    particle.dx += random.uniform(-1, 2)  # Add a small random offset to the x-component of the velocity
                    particle.y = block.y - 5 # Move the particle out of the block
                    break

    def draw(self, screen):
        for block in self.blocks:
            block.draw(screen)


class MovingBar:
    def __init__(self, screen_width, screen_height, bar_width=400, bar_height=20):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.bar_width = bar_width
        self.bar_height = bar_height
        self.x = screen_width // 2
        self.y = screen_height - bar_height
        self.speed = 20
        self.hand_controller = HandController()
        self.direction = None
        self.thread = threading.Thread(target=self.update_direction)
        self.thread.start()

    def update_direction(self, direction):
        self.direction = direction

    def draw(self, screen):
        pygame.draw.rect(screen, (255, 255, 255), pygame.Rect(self.x, self.y, self.bar_width, self.bar_height))

    def update(self):
        if self.direction == 'left' and self.x - self.speed >= 0:
            self.x -= self.speed
        elif self.direction == 'right' and self.x + self.bar_width + self.speed <= self.screen_width:
            self.x += self.speed
     
class Game:
    def __init__(self, width, height, grid_size, direction, num_particles, speed):
        pygame.init()
        self.screen = pygame.display.set_mode((width, height))
        self.mode = 'autopilot'
        self.hand_controller = HandController()
        self.width = width
        self.height = height
        self.moving_bar = MovingBar(self.width, self.height) 

        # Frame rate control
        self.frame_rate = 30  # Adjust this value as needed
        self.last_print_time = time.time()
        self.print_interval = 1.0  # Adjust this value as needed

        # Create the main particle
        self.main_particle = Particle(width // 2, height // 2, speed * 2, direction, (255, 255, 255))

        # Create the particle emitter with a reference to the main particle
        self.emitter = ParticleEmitter(self.width, self.height, 25, self.main_particle)


        # Define the particles attribute
        self.particles = [Particle(i % grid_size * (width // grid_size), 
                           i // grid_size * (height // grid_size), 
                           speed * 2,
                           direction,  # Add direction
                           (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)))  # color
                  for i in range(num_particles)]
        
        self.output_arrays = []
        
    def run(self):
        running = True
        emitter = None
        block_emitter = BlockEmitter(self.width, self.height, 50, 16)
        frame_counter = 0
        N = 32
        GRAVITY = 0.2

        # Set print options
        np.set_printoptions(threshold=np.inf, linewidth=np.inf)
        
        while running:
            # Fill the screen with black
            self.screen.fill((0, 0, 0))

            # Draw the particles
            for particle in self.particles:
                pygame.draw.circle(self.screen, (255, 255, 255), (int(particle.x), int(particle.y)), 20)

            # Update and draw the moving bar
            self.moving_bar.update()
            self.moving_bar.draw(self.screen)

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
                        block_emitter.trigger(50, 100, 50)  # Emit 50 blocks

            if self.mode == 'autopilot':
                for particle in self.particles:
                    # Update the acceleration based on the direction of the particle's movement
                    #particle.update_acceleration(-particle.dx, -particle.dy + GRAVITY)
                    #particle.update_acceleration(random.uniform(-1.0, 1.0), random.uniform(-3.0, 3.0))  # Add random acceleration to make the particle move faster in auto mode
                    particle.update_acceleration(0, 0.5)  # Apply slight downward acceleration for gravity
                    particle.move(self.width, self.height)
            
            elif self.mode == 'manual':
                direction = self.hand_controller.get_direction()
                self.moving_bar.update_direction(direction)  # Update the moving bar direction based on the hand direction

            # Move the particles regardless of the mode
            for particle in self.particles:
                #particle.dx *= 1.5
                #particle.dy *= 1.5
                #particle.update_acceleration(-particle.dx, -particle.dy + GRAVITY)  # Add this line
                particle.update_acceleration(0, 0.5)  # Apply slight downward acceleration for gravity
                particle.speed = 5  # Set the speed to a constant value
                particle.move(self.width, self.height)

                # Check for collision with the moving bar
                if (self.moving_bar.x < particle.x < self.moving_bar.x + self.moving_bar.bar_width and
                    self.moving_bar.y < particle.y < self.moving_bar.y + self.moving_bar.bar_height):
                    particle.dy *= -2  # Reverse the y-component of the velocity
                    particle.dx *= -1
                    particle.y = self.moving_bar.y - 20  # Move the particle out of the bar
                    particle.color = (255, 0, 0)  # Change the color of the particle to red


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

            # Rotate the array by 90 degrees
            rotated_array = np.rot90(reduced_color_array, -1)
            # Flip the array across the vertical axis
            flipped_array = np.fliplr(rotated_array)

            # Store the array in self.output_arrays
            self.output_arrays.append(flipped_array)
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
                threading.Thread(target=visualize_array, args=(flipped_array,)).start()


        self.hand_controller.release()
        pygame.quit()

if __name__ == "__main__":
    game = Game(600, 600, 20, 1, 1, 5) # width, height, grid_size, direction, num_particles, speed
    game.run()
    game.get_output_arrays()