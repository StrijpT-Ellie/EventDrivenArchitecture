import cv2

# Load the trained HOG model for people detection
HOGCV = cv2.HOGDescriptor()
HOGCV.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())

cap = cv2.VideoCapture(0)  # 0 for webcam

while True:
    r, frame = cap.read()
    if r:
        # Resize the frame
        frame = cv2.resize(frame, (640, 480))

        # Detect people in the frame
        boxes, weights = HOGCV.detectMultiScale(frame, winStride=(4, 4), padding=(8, 8), scale=1.05)

        # Draw bounding boxes around detected people
        for (x, y, w, h) in boxes:
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

            # Print the bounding box coordinates to the terminal
            print(f"Detected person at coordinates: X={x}, Y={y}, Width={w}, Height={h}")

        # Show the frame
        cv2.imshow("preview", frame)

    # Exit if ESC key is pressed
    if cv2.waitKey(1) == 27:
        break

# Release the VideoCapture object
cap.release()

# Close all OpenCV windows
cv2.destroyAllWindows()