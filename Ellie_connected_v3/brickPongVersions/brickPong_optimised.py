#this code is not optimised, rather it uses cProfile and creates a file for profiling which can be viewed in snakeviz

import threading
import pygame
import random
import cv2
import numpy as np
import mediapipe as mp
import math
import time
from datetime import datetime
import cProfile

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

            movement_threshold = 0.001

            if self.old_hand_landmarks is not None:
                dx = hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].x - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].x
                dy = hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].y - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.WRIST].y
                if abs(dx) > movement_threshold or abs(dy) > movement_threshold:
                    if abs(dx) > abs(dy):
                        direction = 'right' if dx > 0 else 'left'
                    else:
                        direction = 'down' if dy > 0 else 'up'

            self.old_hand_landmarks = hand_landmarks

        return direction

    def release(self):
        self.cap.release()
        self.hands.close()

class Particle:
    def __init__(self, x, y, speed, direction, color, lifespan=None):
        self.x = x
        self.y = y
        self.speed = speed
        self.color = color
        self.direction = math.radians(direction)
        self.dx = self.speed * math.cos(self.direction)
        self.dy = self.speed * math.sin(self.direction)
        self.ax = random.uniform(-1, 0)
        self.ay = random.uniform(-1, 1)
        self.damping = 0.98
        self.lifespan = lifespan

    def move(self, width, height):
        self.dx += self.ax
        self.dy += self.ay
        
        self.dx *= self.damping
        self.dy *= self.damping

        next_x = self.x + self.dx
        next_y = self.y + self.dy

        if self.lifespan is not None:
            self.lifespan -= 1

        if next_x < 0 or next_x > width:
            self.dx *= -1
        if next_y < 0 or next_y > height:
            self.dy *= -1

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
        self.main_particle = main_particle

    def generate_particle(self):
        x = self.screen_width // 2
        y = self.screen_height // 2
        speed = random.randint(1, 10)
        direction = random.randint(0, 360)
        color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
        lifespan = 300
        return Particle(x, y, speed, direction, color, lifespan)

    def trigger(self):
        if len(self.particles) < self.num_particles:
            for _ in range(self.num_particles - len(self.particles)):
                self.particles.append(self.generate_particle())

    def update(self, blocks):
        blocks_to_remove = []
        particles_to_remove = []

        for particle in self.particles:
            particle.move(self.screen_width, self.screen_height)

            if self.check_collision(particle, self.main_particle):
                particle.dx *= -1
                particle.dy *= -1

            for block in blocks:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    blocks_to_remove.append(block)
                    particle.dx *= -1
                    particle.dy *= -1
                    break

            if particle.x < 0 or particle.y < 0 or particle.x > self.screen_width or particle.y > self.screen_height:
                particles_to_remove.append(particle)

            if particle.lifespan <= 0:
                particles_to_remove.append(particle)

        for block in blocks_to_remove:
            if block in blocks:
                blocks.remove(block)

        for particle in particles_to_remove:
            if particle in self.particles:
                self.particles.remove(particle)

    def draw(self, screen):
        for particle in self.particles:
            pygame.draw.circle(screen, particle.color, (int(particle.x), int(particle.y)), 20)

    def check_collision(self, particle1, particle2):
        distance = math.sqrt((particle1.x - particle2.x)**2 + (particle1.y - particle2.y)**2)
        return distance < 12

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
        self.last_destroyed_time = None

    def trigger(self, num_blocks, block_width, block_height):
        for i in range(num_blocks):
            row = i // self.blocks_per_row
            column = i % self.blocks_per_row
            x = column * block_width
            y = row * block_height
            color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            block = Block(x, y, block_width, block_height, color)
            self.blocks.append(block)
        self.last_destroyed_time = None

    def update(self, particles):
        blocks_to_remove = []
        for block in self.blocks[:]:
            for particle in particles:
                if block.x < particle.x < block.x + block.width and block.y < particle.y < block.y + block.height:
                    blocks_to_remove.append(block)
                    particle.dy *= -1
                    particle.dx += random.uniform(-4, 10)
                    break

        for block in blocks_to_remove:
            self.blocks.remove(block)

        if not self.blocks and self.last_destroyed_time is None:
            self.last_destroyed_time = time.time()

    def draw(self, screen):
        for block in self.blocks:
            block.draw(screen)

class MovingBar:
    def __init__(self, screen_width, screen_height, bar_width=400, bar_height=20):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.bar_width = bar_width
        self.bar_height = bar_height
        self.x = screen_width // 2 - bar_width // 2
        self.y = screen_height - bar_height - 10
        self.speed = 20
        self.hand_controller = HandController()
        self.direction = None
        self.thread = threading.Thread(target=self.update_direction_thread)
        self.thread.daemon = True
        self.thread.start()
        self.movement_log = []
        self.log_file = open("movement_log.txt", "w")  # Open the file in write mode

    def log_movement(self):
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        log_entry = f"{timestamp}, {self.x}, {self.y}\n"
        self.movement_log.append(log_entry)
        self.log_file.write(log_entry)  # Write the log entry to the file
        self.log_file.flush()  # Ensure it gets written to disk
        print(log_entry)  # Debug statement to print to the terminal

    def update_direction(self, direction):
        self.direction = direction

    def update_direction_thread(self):
        while True:
            direction = self.hand_controller.get_direction()
            self.update_direction(direction)

    def draw(self, screen):
        pygame.draw.rect(screen, (255, 255, 255), pygame.Rect(self.x, self.y, self.bar_width, self.bar_height))

    def update(self):
        if self.direction == 'left' and self.x - self.speed >= 0:
            self.x -= self.speed
            self.log_movement()
        elif self.direction == 'right' and self.x + self.bar_width + self.speed <= self.screen_width:
            self.x += self.speed
            self.log_movement()

    def clear_log_file(self):
        self.log_file.close()
        open("movement_log.txt", "w").close()  # Clear the log file

class Game:
    def __init__(self, width, height, grid_size, direction, num_particles, speed):
        pygame.init()
        self.screen = pygame.display.set_mode((width, height))
        self.width = width
        self.height = height
        self.hand_controller = HandController()
        self.moving_bar = MovingBar(self.width, self.height)

        self.frame_rate = 5 #original rate 30
        self.last_print_time = time.time()
        self.print_interval = 0.5 #original was 1

        self.main_particle = Particle(width // 2, height // 2, speed * 2, direction, (255, 255, 255))
        self.emitter = ParticleEmitter(self.width, self.height, 20, self.main_particle)

        self.particles = [Particle(self.width // 2, self.height // 2, speed * 2, direction,
                           (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)), 300)
                  for _ in range(num_particles)]

        self.output_arrays = []
        self.destroyed_bricks = 0
        self.emitter_triggered = False

    def run(self):
        running = True
        block_emitter = BlockEmitter(self.width, self.height, 50, 16)
        block_emitter.trigger(100, 40, 20)
        frame_counter = 0
        N = 32

        np.set_printoptions(threshold=np.inf, linewidth=np.inf)

        while running:
            self.screen.fill((0, 0, 0))

            for particle in self.particles:
                pygame.draw.circle(self.screen, (255, 255, 255), (int(particle.x), int(particle.y)), 20)

            self.emitter.draw(self.screen)
            self.moving_bar.update()
            self.moving_bar.draw(self.screen)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_q:
                        running = False

            direction = self.hand_controller.get_direction()
            self.moving_bar.update_direction(direction)

            for particle in self.particles:
                particle.update_acceleration(0, 0.5)
                particle.move(self.width, self.height)

                if (self.moving_bar.x < particle.x < self.moving_bar.x + self.moving_bar.bar_width and
                    self.moving_bar.y < particle.y < self.moving_bar.y + self.moving_bar.bar_height):
                    particle.dy *= -2
                    particle.dx += random.uniform(-4, 10)
                    particle.y = self.moving_bar.y - 20
                    particle.color = (255, 0, 0)

            destroyed_bricks_before = len(block_emitter.blocks)
            block_emitter.update(self.particles)
            block_emitter.draw(self.screen)
            destroyed_bricks_after = len(block_emitter.blocks)

            self.destroyed_bricks += destroyed_bricks_before - destroyed_bricks_after

            self.emitter.update(block_emitter.blocks)

            if self.destroyed_bricks >= 20 and not self.emitter_triggered:
                self.emitter.trigger()
                self.destroyed_bricks = 0
                self.emitter_triggered = True

            if self.emitter_triggered and len(self.emitter.particles) == 0:
                self.emitter_triggered = False

            if block_emitter.last_destroyed_time and (time.time() - block_emitter.last_destroyed_time >= 10):
                block_emitter.trigger(100, 40, 20)
                print("Blocks re-emitted after 10 seconds.")
                block_emitter.last_destroyed_time = None

            screen_array = pygame.surfarray.array3d(self.screen)
            resized_array = cv2.resize(screen_array, (20, 20))
            reduced_color_array = (resized_array / 32).astype(np.uint8) * 32
            rotated_array = np.rot90(reduced_color_array, -1)
            flipped_array = np.fliplr(rotated_array)

            self.output_arrays.append(flipped_array)
            if time.time() - self.last_print_time >= self.print_interval:
                print(reduced_color_array)
                self.last_print_time = time.time()

            pygame.time.Clock().tick(self.frame_rate)
            pygame.display.flip()
            frame_counter += 1

            # if frame_counter % N == 0:
            #     threading.Thread(target=visualize_array, args=(flipped_array,)).start()

        #self.moving_bar.close_log_file()
        self.moving_bar.clear_log_file()
        self.hand_controller.release()
        pygame.quit()

if __name__ == "__main__":
    profiler = cProfile.Profile()
    profiler.enable()
    
    game = Game(600, 600, 20, 1, 2, 10)
    game.run()
    
    profiler.disable()
    profiler.dump_stats('game_profile.prof')
