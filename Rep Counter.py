#   Rep Counter for working out
#       Version = 0.3


import time
import os
import pygame

from os import times_result


pygame.mixer.init()

def countdown(time_sec):
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
    pygame.mixer.music.load("Rep Counter\sounds\mother-fucker.mp3")
    pygame.mixer.music.play()
else:
    while reps < totalreps:
        
        
        print("\nYou have completed " + str(reps) + " reps\n")
        

        
        a=int(input("How long till next Rep in minutes?\n\n"))
        
        countdown(a*60)
        
        print("\nStart your next Rep")
        pygame.mixer.music.load("Rep Counter\sounds\loud-noises!.mp3")
        pygame.mixer.music.play()
        
        
        reps = reps+1
        
        time.sleep(10)
        
        os.system("cls" if os.name == "nt" else "clear")
    
    if reps == totalreps:
        print("Congrats you are done working out!!!\n")

print("Closing window in:")

countdown(60)

exit()