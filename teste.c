#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void r_val(){
    for(int i=0; i<3; i++){
        int random_val = rand() % 3;
        printf("random_val: %d\n", random_val);
   }
}

int main(){
   srand(time(NULL));
   r_val();
}