"""
Rep Counter for working out

This code snippet is a part of a program that acts as a rep counter for a workout. It takes user input for the number of reps to be done and the duration between each rep. It then displays the progress, plays sound effects, and counts down the time until the next rep. Once all the reps are completed, it displays a congratulatory message and closes the window after a countdown.

Example Usage:
```python
How many Reps are you doing today?
3

You have completed 0 reps

How long till next Rep in minutes?
2

02 : 00
Start your next Rep

You have completed 1 reps

How long till next Rep in minutes?
3

03 : 00
Start your next Rep

You have completed 2 reps

How long till next Rep in minutes?
1

01 : 00
Start your next Rep

Congrats you are done working out!!!

Closing window in:
01 : 00
```

Inputs:
- The number of reps to be done (amounts)
- The duration between each rep in minutes (a)

Flow:
1. The program initializes the necessary modules and functions.
2. It clears the console screen.
3. The user is prompted to enter the number of reps to be done.
4. The total number of reps is updated based on the user input.
5. If the total number of reps is 0, a message is displayed and a sound effect is played.
6. If the total number of reps is not 0, a loop is executed until the number of completed reps is equal to the total number of reps.
7. Inside the loop, the number of completed reps is displayed.
8. The user is prompted to enter the duration until the next rep.
9. A countdown is displayed for the specified duration.
10. A sound effect is played to indicate the start of the next rep.
11. The number of completed reps is incremented.
12. A delay of 10 seconds is added.
13. The console screen is cleared.
14. After the loop ends, a congratulatory message is displayed if the number of completed reps is equal to the total number of reps.
15. A countdown is displayed before the program exits.

Outputs:
- Progress messages indicating the number of completed reps.
- Countdowns for the duration until the next rep and before the program exits.
"""

import time
import os
import pygame

from os import times_result

pygame.mixer.init()

def countdown(time_sec):
    """
    Countdown function

    This function takes a time in seconds and displays a countdown in minutes and seconds format.
    It uses the time.sleep() function to pause the program for 1 second between each countdown update.

    Parameters:
    - time_sec (int): The time in seconds to countdown from

    Returns:
    - None
    """
    while time_sec:
        mins, sec = divmod(time_sec, 60)
        timeformat = '{:02d} : {:02d}'.format(mins, sec)
        print(timeformat, end="\r")
        time.sleep(1)
        time_sec -=1

os.system("cls" if os.name == "nt" else "clear")

totalreps=0
reps = 0

amounts=int(input("How many Reps are you doing today?\n"))

totalreps = totalreps + amounts

if totalreps == 0:
    print("Wow u a lazy bitch......")
    pygame.mixer.music.load("sounds\mother-fucker.mp3")
    pygame.mixer.music.play()
else:
    while reps < totalreps:
        print("\nYou have completed " + str(reps) + " reps\n")
        a=int(input("How long till next Rep in minutes?\n\n"))
        countdown(a*60)
        print("\nStart your next Rep")
        pygame.mixer.music.load("sounds\loud-noises!.mp3")
        pygame.mixer.music.play()
        reps = reps+1
        time.sleep(10)
        os.system("cls" if os.name == "nt" else "clear")
    
    if reps == totalreps:
        print("Congrats you are done working out!!!\n")

print("Closing window in:")
countdown(60)

exit()
"""