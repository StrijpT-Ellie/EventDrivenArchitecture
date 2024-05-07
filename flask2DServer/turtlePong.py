# Import module
import turtle


# Set up game window
screen = turtle.Screen()
screen.title('Pong')
screen.setup(width=800, height=600)
screen.tracer(0)

# Creating the players
# Player A
playerA = turtle.Turtle()
playerA.penup()
playerA.setpos(-300, 0)
playerA.shape('square')
playerA.shapesize(3, 1, 1)
playerAScore = 0

#Player B
playerB = turtle.Turtle()
playerB.penup()
playerB.setpos(300, 0)
playerB.shape('square')
playerB.shapesize(3, 1, 1)
playerBScore = 0

# Creating the ball
ball = turtle.Turtle()
ball.penup()
ball.shape('square')
ball.shapesize(0.5, 0.5, 0.5)
ball.dx = 1
ball.dy = 0.5


# Creating a pen to write the score 
pen = turtle.Turtle()
pen.penup()
pen.goto(0, 260)
pen.hideturtle()
pen.write('0             0', align='center', font=('Courier', 30, ' normal'))



# Player Movement
# Move down
def moveDown():
    y = playerA.ycor()
    y -= 15
    playerA.sety(y)

# Move up
def moveUp():
    y = playerA.ycor()
    y += 15
    playerA.sety(y)


def moveDownB():
    y = playerB.ycor()
    y -= 15
    playerB.sety(y)

# Move up
def moveUpB():
    y = playerB.ycor()
    y += 15
    playerB.sety(y)

def restartGame():
    ball.dx = 1
    ball.dy = 0.5


# Key bindings 
screen.listen()
screen.onkey(moveDown, 's')
screen.onkey(moveUp, 'w')
screen.onkey(moveDownB, 'Down')
screen.onkey(moveUpB, 'Up')
screen.onkey(restartGame, 'space')


# Main game loop
while True:
    screen.update()



    # Ball Movement
    ball.setx(ball.xcor() + ball.dx)
    ball.sety(ball.ycor() + ball.dy)



    # Ball collision with walls
    if ball.ycor() < -295:
        ball.dy *= -1
    elif ball.ycor() > 295:
        ball.dy *= -1

    
    # Ball collision with player
    if (ball.xcor() < -290 and ball.xcor() > -305) and (ball.ycor() < playerA.ycor()+30 and ball.ycor() > playerA.ycor()-30):
        ball.setx(-290)
        ball.dx *= -1
    
    if (ball.xcor() > 290 and ball.xcor() < 305) and (ball.ycor() < playerB.ycor()+30 and ball.ycor() > playerB.ycor()-30):
        ball.setx(290)
        ball.dx *= -1


    # Scores 
    if ball.xcor() < -395:
        ball.goto(0,0)
        ball.dx *= 0
        ball.dy *= 0
        playerBScore += 1
        pen.clear()
        pen.write('{}             {}'.format(playerAScore, playerBScore), align='center', font=('Courier', 30, ' normal'))
    elif ball.xcor() > 395:
        ball.goto(0,0)
        ball.dx *= 0
        ball.dy *= 0
        playerAScore += 1
        pen.clear()
        pen.write('{}             {}'.format(playerAScore, playerBScore), align='center', font=('Courier', 30, ' normal'))


    # Player boundaries
    if playerA.ycor() >= 300:
        playerA.sety(295)
    if playerA.ycor() <= -300:
        playerA.sety(-295)
    
    if playerB.ycor() >= 300:
        playerB.sety(295)
    if playerB.ycor() <= -300:
        playerB.sety(-295)