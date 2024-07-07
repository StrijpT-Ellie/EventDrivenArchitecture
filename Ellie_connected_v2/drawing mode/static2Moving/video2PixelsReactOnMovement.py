#This script is used to detect movement in the camera feed and display it on the LED grid.
#The script uses OpenCV to detect movement and Pygame to display the camera feed on the LED grid.
#The movement is displayed in red on the LED grid.

import pygame
import cv2
import numpy as np

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

def draw_grid(screen, grid):
    for y in range(GRID_SIZE):
        for x in range(GRID_SIZE):
            color = grid[y][x]
            pygame.draw.circle(screen, color, (x * PIXEL_SIZE + PIXEL_SIZE // 2, y * PIXEL_SIZE + PIXEL_SIZE // 2), PIXEL_SIZE // 2 - 1)

def detect_movement(prev_frame, current_frame, threshold=25):
    if prev_frame is None:
        return None
    diff = cv2.absdiff(prev_frame, current_frame)
    gray = cv2.cvtColor(diff, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, threshold, 255, cv2.THRESH_BINARY)
    return thresh

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

    grid = [[BLACK for _ in range(GRID_SIZE)] for _ in range(GRID_SIZE)]

    for y in range(GRID_SIZE):
        for x in range(GRID_SIZE):
            if movement_mask is not None and movement_mask[y, x] > 0:
                grid[y][x] = (255, 0, 0)  # Red for movement
            else:
                grid[y][x] = frame_rgb[y, x]

    prev_frame = frame.copy()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    screen.fill(BLACK)
    draw_grid(screen, grid)
    pygame.display.flip()

cap.release()
pygame.quit()
