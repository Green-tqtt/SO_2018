#include "header.h"
int world_cord_x;
int world_cord_y;
Warehouse *sample;
ProductTypeList productType;

Stats *stats_ptr;
Warehouse *sample;
pthread_t drone_thread[4]; //Número de Drones
int power = 1;
int shmid;

int main(){
	signal(SIGINT, signal_handler);
    
    printf("oi");
    Warehouse *list[2];
	sample = (Warehouse *)malloc(sizeof(Warehouse));
    sample->w_x = 200;

    list[1]=sample;

    printf("\nsample x : %f\n", list[1]->w_x);
    read_config();
    create_thread_pool();
    create_shared_memory();
    while(power){


    }
}

void signal_handler(int signum){
    while(wait(NULL)>0);
    //shutdown_semaphores();
    //stats_results();
    power = 0;
    destroy_shared_memory();
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

void create_shared_memory(){
    if((shmid = shmget(IPC_PRIVATE, sizeof(Stats), IPC_CREAT |0766)) == -1){
        perror("Error creating shared memory\n");   
        exit(1);
    }
    stats_ptr = (Stats*) shmat(shmid, NULL, 0);
    printf("----------SHARED MEMORY----------\n");
    printf("\nShared memory sucessfully at address %p\n", stats_ptr);
    printf("\nShared memory at %d\n", shmid);
    printf("---------------------------------\n");
    
    
    stats_ptr->n_e_drones=0;
    stats_ptr->n_p_warehouse=0;
    stats_ptr->n_e_delivered=0;
    stats_ptr->n_p_delivered=0;
    stats_ptr->average_time=0.0;
    //Dados teste

    printf("----------RESULTS----------\n");
    printf("Número de encomendas atribuídas a drones: %d\n", stats_ptr->n_e_drones);
    printf("Número de produtos carregados nos armazéns: %d\n", stats_ptr->n_p_warehouse);
    printf("Número de encomendas entregues: %d\n", stats_ptr->n_e_delivered);
    printf("Número de produtos entregues: %d\n", stats_ptr->n_p_delivered);
    printf("Tempo médio para conclusão de uma encomenda: %.1f\n", stats_ptr->average_time);
    printf("---------------------------\n");
	
}

void destroy_shared_memory(){
    if(shmdt(stats_ptr) == -1){
        perror("Error using shmdt\n");
    }
    printf("Sucessfully shmdt'd\n");
    if(shmctl(shmid, IPC_RMID, NULL) == -1){
        perror("Error unmapping shared memory\n");
    }
    printf("Sucessfully shmctl'd\n");
}


void read_config(){
    FILE *fp = fopen("config.txt", "r");
    if (!fp){
        perror("Error reading the file\n");
        exit(0);
    }
    else
        printf("File found\n");
    char line[255];
    fgets(line, sizeof(line), fp);
    char wc_x[50], wc_y[50];
    strcpy(wc_x, strtok(line, ","));
    strcpy(wc_y, strtok(NULL, ","));
    printf("World cord x: %s\n", wc_x);
    printf("World cord y: %s\n", wc_y);
    /*Warehouse *warehouseList[2];
    sample = (Warehouse*) malloc(sizeof(Warehouse));
    sample->w_x = 20;
    sample->w_y = 30;
    warehouseList[0] = sample;
    printf("Warehouse sdds %f\n", sample->w_y);*/
    char *token;
    char p_name[50];
    productType = create_product_type_list();
    fgets(line, sizeof(line), fp);
    //ir buscar primeira ","
    token = strtok(line, ",");
    while(token != NULL){
        if(token[0] == ' '){
            token++;
        }
        if(token[strlen(token)-1] == '\n'){
            token[strlen(token)-1] = '\0';
        }
        strcpy(p_name, token);
        insert_product_type(p_name, productType);
        token = strtok(NULL, ",");
    }
    fclose(fp);

}

//cria lista ligada de product types
ProductTypeList create_product_type_list(void){

    ProductTypeList type_node;
    type_node = (ProductTypeList) malloc(sizeof(type_node));

    if(type_node != NULL){
        type_node->next = NULL;
    }
    printf("List created\n");
    return type_node;
}
//insere product no array list de product types
void insert_product_type(char p_name[50], ProductTypeList productType){
    ProductTypeList insertProduct;
    ProductTypeList atual = productType;;
    insertProduct = malloc(sizeof(type_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    strcpy(insertProduct->product_type.p_name, p_name);
    atual->next = insertProduct;
    insertProduct->next = NULL;
}

void list_product_types(ProductTypeList productType){
    ProductTypeList node;
    node = productType->next;
    while(node != NULL){
        printf("Type: %s\n", node->product_type.p_name);
        node = node->next;
    } 
}
