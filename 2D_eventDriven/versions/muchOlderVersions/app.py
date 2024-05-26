import pygame
import random

class Particle:
    def __init__(self, x, y, speed):
        self.x = x
        self.y = y
        self.dx = random.choice([-speed, speed])
        self.dy = random.choice([-speed, speed])

    def move(self, width, height):
        self.x = (self.x + self.dx) % width
        self.y = (self.y + self.dy) % height

class Game:
    def __init__(self, width, height, grid_size, num_particles, speed):
        self.width = width
        self.height = height
        self.grid_size = grid_size
        self.num_particles = num_particles
        self.speed = speed

        pygame.init()
        self.screen = pygame.display.set_mode((self.width, self.height))

        self.particles = [Particle(i % self.grid_size * (self.width // self.grid_size), 
                                   i // self.grid_size * (self.height // self.grid_size), 
                                   self.speed) 
                          for i in range(self.num_particles)]

    def run(self):
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

            for particle in self.particles:
                particle.move(self.width, self.height)

            self.screen.fill((0, 0, 0))

            for particle in self.particles:
                pygame.draw.circle(self.screen, (255, 255, 255), (particle.x, particle.y), 5)

            pygame.display.flip()

        pygame.quit()

if __name__ == "__main__":
    game = Game(800, 600, 20, 2, 1)
    game.run()