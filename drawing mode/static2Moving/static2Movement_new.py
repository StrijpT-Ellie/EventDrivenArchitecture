import pygame
import cv2
import numpy as np
import random

# Initialize Pygame
pygame.init()

# Constants
WIDTH, HEIGHT = 600, 600
GRID_SIZE = 20
PIXEL_SIZE = WIDTH // GRID_SIZE

# Colors
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

# Initialize the screen
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("20x20 LED Grid")

# Initialize OpenCV
cap = cv2.VideoCapture(0)

class Particle:
    def __init__(self, x, y, color):
        self.x = x
        self.y = y
        self.color = color
        self.vel_x = random.uniform(-1, 1)
        self.vel_y = random.uniform(-1, 1)
        self.original_pos = (x, y)
        self.moving = False

    def update(self):
        if self.moving:
            self.x += self.vel_x
            self.y += self.vel_y

            # Bounce off the edges
            if self.x < 0 or self.x > WIDTH:
                self.vel_x *= -1
            if self.y < 0 or self.y > HEIGHT:
                self.vel_y *= -1

    def draw(self, screen):
        pygame.draw.circle(screen, self.color, (int(self.x), int(self.y)), PIXEL_SIZE // 2 - 1)

def draw_grid(screen, particles):
    for particle in particles:
        particle.draw(screen)

def detect_movement(prev_frame, current_frame, threshold=25):
    if prev_frame is None:
        return None
    diff = cv2.absdiff(prev_frame, current_frame)
    gray = cv2.cvtColor(diff, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, threshold, 255, cv2.THRESH_BINARY)
    return thresh

# Initialize particles
particles = []
for y in range(GRID_SIZE):
    for x in range(GRID_SIZE):
        particles.append(Particle(x * PIXEL_SIZE + PIXEL_SIZE // 2, y * PIXEL_SIZE + PIXEL_SIZE // 2, BLACK))

# Main loop
running = True
prev_frame = None
while running:
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.resize(frame, (GRID_SIZE, GRID_SIZE))
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    movement_mask = detect_movement(prev_frame, frame)

    for y in range(GRID_SIZE):
        for x in range(GRID_SIZE):
            idx = y * GRID_SIZE + x
            if movement_mask is not None and movement_mask[y, x] > 0:
                particles[idx].color = (255, 0, 0)  # Red for movement
                particles[idx].moving = True
            else:
                particles[idx].color = frame_rgb[y, x]
                particles[idx].moving = False
                # Reset position if no movement
                particles[idx].x, particles[idx].y = particles[idx].original_pos

    prev_frame = frame.copy()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    screen.fill(BLACK)
    draw_grid(screen, particles)
    pygame.display.flip()

    for particle in particles:
        particle.update()

cap.release()
pygame.quit()
