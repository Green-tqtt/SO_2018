#include "header.h"

Warehouse *sample;
pthread_t drone_thread[4]; //Número de Drones
int power = 1;

int main(){
	signal(SIGINT, signal_handler);
    printf("oi");
    Warehouse *list[2];
	sample = (Warehouse *)malloc(sizeof(Warehouse));
    sample->w_x = 200;

    list[1]=sample;

    printf("\nsample x : %d\n", list[1]->w_x);

    create_thread_pool();
    while(power){
    	

    }
}

void signal_handler(int signum){
    while(wait(NULL)>0);
    //shutdown_semaphores();
    //stats_results();
    //cleanup_sm();
    power = 0;
    destroy_thread_pool();
    free(sample);
    kill(0, SIGKILL);
    exit(0);
}

void create_thread_pool(){
	int i;
    int drone_id[4]; // Número de Drones
    for(i = 0; i < 4; i++){
        drone_id[i] = i;
        if(pthread_create(&drone_thread[i], NULL, drone_action, &drone_id[i])==0){
            printf("Drone thread %d was created\n", drone_id[i]);
            //drone_stats();
        }
        else{
            perror("Error creating Drone thread\n");
        }
    }
}

void destroy_thread_pool(){
	int drone_id[4];
	int i;
    for(i = 0; i < 4; i++){
        drone_id[i] = i;
        if(pthread_join(drone_thread[i], NULL)==0){
            printf("Drone thread %d was killed\n", drone_id[i]);
        }
        else{
            perror("Error killing Drone thread\n");
        }
    }

}

void *drone_action(){
    printf("Hello! I'm a thread!\n");
    printf("Tchillando\n");
    while(power){

    }
    printf("Ja nao tchillo nada\n");
    printf("Thread is leaving... :(\n");
    pthread_exit(NULL);
}