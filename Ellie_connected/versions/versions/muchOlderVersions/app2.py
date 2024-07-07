#This script 

import pygame
import random
import cv2
import numpy as np
from pyswarm import pso

def objective_func(params):
        total_distance = 0
        for i in range(0, len(params), 2):
            speed = params[i]
            direction = params[i + 1]
            distance = speed  # For simplicity, let's assume that each step moves one unit in the direction
            total_distance += distance
        return total_distance
    
class SwarmController:
    def __init__(self, num_particles, speed_bounds, direction_bounds):
        self.num_particles = num_particles
        self.speed_bounds = speed_bounds
        self.direction_bounds = direction_bounds

    def optimize(self, objective_func):
        lb = [self.speed_bounds[0], self.direction_bounds[0]] * self.num_particles
        ub = [self.speed_bounds[1], self.direction_bounds[1]] * self.num_particles

        xopt, fopt = pso(objective_func, lb, ub)

        return xopt

class HandController:
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        self.width = int(self.cap.get(3))
        self.height = int(self.cap.get(4))
        self.old_gray = None
        self.p0 = None

    def get_direction(self):
        ret, frame = self.cap.read()
        frame = cv2.flip(frame, 1)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        if self.old_gray is None:
            self.old_gray = gray
            self.p0 = cv2.goodFeaturesToTrack(self.old_gray, mask=None, **feature_params)
            return 'none'

        p1, st, err = cv2.calcOpticalFlowPyrLK(self.old_gray, gray, self.p0, None, **lk_params)

        if p1 is not None:
            dx, dy = np.mean(p1 - self.p0, axis=0)[0]
            if abs(dx) > abs(dy):
                direction = 'right' if dx > 0 else 'left'
            else:
                direction = 'down' if dy > 0 else 'up'
        else:
            direction = 'none'

        self.old_gray = gray.copy()
        self.p0 = cv2.goodFeaturesToTrack(self.old_gray, mask=None, **feature_params)

        return direction

    def release(self):
        self.cap.release()

# Parameters for ShiTomasi corner detection
feature_params = dict(maxCorners=1000, qualityLevel=0.01, minDistance=7, blockSize=7)

# Parameters for Lucas-Kanade optical flow
lk_params = dict(winSize=(5, 5), maxLevel=2, criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

class Particle:
    def __init__(self, x, y, speed, direction, color):
        self.x = x
        self.y = y
        self.speed = speed 
        self.color = color 
        
        self.direction = direction
        self.dx = random.choice([-speed, speed])
        self.dy = random.choice([-speed, speed])

    def move(self, width, height):
        self.x = (self.x + self.dx) % width
        self.y = (self.y + self.dy) % height
        
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
            particle.x -= particle.speed
            if particle.x < 0:
                self.particles.remove(particle)

    def draw(self, screen):
        for particle in self.particles:
            pygame.draw.circle(screen, particle.color, (particle.x, particle.y), 5)
            
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

    def run(self):
        
        running = True
        emitter = None
        swarm_controller = None
        
        while running:
            # Fill the screen with black
            self.screen.fill((0, 0, 0))

            # Draw the particles
            for particle in self.particles:
                pygame.draw.circle(self.screen, (255, 255, 255), (int(particle.x), int(particle.y)), 5)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_SPACE:  # Press space to switch mode
                        self.mode = 'manual' if self.mode == 'autopilot' else 'autopilot'
                    elif event.key == pygame.K_p:  # Press "p" to trigger the emitter
                        emitter = ParticleEmitter(self.width, self.height, 25)
                        emitter.trigger()
                    elif event.key == pygame.K_s:  # Press "s" to trigger the swarm controller
                        swarm_controller = SwarmController(20, (1, 5), (0, 360))
                        optimal_params = swarm_controller.optimize(objective_func)
                        # Update the particles with the optimal parameters

            if self.mode == 'autopilot':
                for particle in self.particles:
                    particle.move(self.width, self.height)
            elif self.mode == 'manual':
                direction = self.hand_controller.get_direction()
                if direction == 'up':
                    for particle in self.particles:
                        particle.y -= particle.speed
                elif direction == 'down':
                    for particle in self.particles:
                        particle.y += particle.speed
                elif direction == 'left':
                    for particle in self.particles:
                        particle.x -= particle.speed
                elif direction == 'right':
                    for particle in self.particles:
                        particle.x += particle.speed
                        
                if emitter:  # If the emitter has been triggered
                    emitter.update()
                    emitter.draw(self.screen)

            pygame.display.flip()

        self.hand_controller.release()
        pygame.quit()

if __name__ == "__main__":
    game = Game(800, 600, 20, 2, 3, 1)
    game.run()