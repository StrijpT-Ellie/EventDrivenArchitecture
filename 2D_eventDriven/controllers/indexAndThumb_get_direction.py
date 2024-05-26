# this was a part of handcontroller oriented on thumb direction recognition

def get_direction(self):
        direction = 'none'  
        ret, frame = self.cap.read()
        frame = cv2.flip(frame, 1)
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = self.hands.process(frame_rgb)

        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                self.mp_draw.draw_landmarks(frame, hand_landmarks, self.mp_hands.HAND_CONNECTIONS)

                thumb_tip = hand_landmarks.landmark[self.mp_hands.HandLandmark.THUMB_TIP]
                index_finger_tip = hand_landmarks.landmark[self.mp_hands.HandLandmark.INDEX_FINGER_TIP]

                distance = ((thumb_tip.x - index_finger_tip.x) ** 2 + (thumb_tip.y - index_finger_tip.y) ** 2) ** 0.5

                if distance < 0.02:  # Adjust this value based on your needs
                    print("Thumb and pointing finger together gesture detected")

                    if self.old_hand_landmarks is not None:
                        dx = thumb_tip.x - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.THUMB_TIP].x
                        dy = thumb_tip.y - self.old_hand_landmarks.landmark[self.mp_hands.HandLandmark.THUMB_TIP].y
                        if abs(dx) > 0.01 or abs(dy) > 0.01:  # Adjust this value to change the sensitivity
                            if abs(dx) > abs(dy):
                                direction = 'right' if dx > 0 else 'left'
                            else:
                                direction = 'down' if dy > 0 else 'up'
                            print(f"Thumb and pointing finger together movement: dx={dx}, dy={dy}, direction={direction}")  # Debug print

                self.old_hand_landmarks = hand_landmarks  # Update old_hand_landmarks

        return direction