#include <stdio.h>
#include <math.h>
#include <time.h>

/*Rep Counter

    Version: alpha 1.0
    Discription: Takes inputs from users to determine the wait time between workout reps they aim to do.

*/

int main(){

    int totalreps= 0;
    int reps = 0;

    printf("How many reps would u like to do?\n"); //Asks for total reps to be completed for this workout.
    scanf("%d", &totalreps);

    int repminutes = 0;

    printf("How many minutes in between each rep?\n"); //Asks for time in between reps. 
    scanf("%d", &repminutes);

    if (totalreps == 0) {
        printf("Wow you lazy bitch.....");
    }
    else if (totalreps < 0)
    {
        printf("Thats not possible.....");
    }
    
    while (totalreps>reps){

            int seconds = repminutes * 60; // Move calculation of 'seconds' inside the while loop

            while (seconds > 0) { //Countdown timer for waiting in between reps.
                int m = seconds / 60;
                int s = seconds % 60;

                printf("\r%02d:%02d", m, s);

                fflush(stdout);

                clock_t stop = clock() + CLOCKS_PER_SEC;
                while (clock() < stop) {}

                seconds--;
            }

            reps++;

            printf("\n\nStart your next rep. Press Enter to continue.\n");
            fflush(stdin);
            getchar();
            sleep(1);

    }
    

   if(reps == totalreps){

    printf("\nYou are done working out!"); //Message for when you are done working out.
    printf("\nClosing window in:\n"); 
    
    int seconds = 15;

        while (seconds > 0) { // Countdown till window closes.

                    printf("\r%02d seconds", seconds);

                    fflush(stdout);

                    clock_t stop = clock() + CLOCKS_PER_SEC;
                    while (clock() < stop) {}

                    seconds--;
        }
    
    
   }

    return 0;    
}
