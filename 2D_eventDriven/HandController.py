

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

            movement_threshold = 0.005  

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
    def __init__(self, x, y, speed, direction, color):
        self.x = x
        self.y = y
        self.speed = speed 
        self.color = color 
        self.direction = direction
        angle_radians = math.radians(direction)
        self.dx = speed * math.cos(angle_radians)
        self.dy = speed * math.sin(angle_radians)
        self.ax = random.uniform(-0.05, 0.05)
        self.ay = random.uniform(-0.05, 0.05)
        self.damping = 0.98

    def move(self, width, height):
        self.dx += self.ax
        self.dy += self.ay
        self.dx *= self.damping
        self.dy *= self.damping

        next_x = self.x + self.dx
        next_y = self.y + self.dy

        if next_x < 0 or next_x > width:
            self.dx *= -1
        if next_y < 0 or next_y > height:
            self.dy *= -1

        self.x += self.dx
        self.y += self.dy

    def update_params(self, speed, direction):
        self.speed = speed
        self.direction = direction
        angle_radians = math.radians(direction)
        self.dx = speed * math.cos(angle_radians)
        self.dy = speed * math.sin(angle_radians)