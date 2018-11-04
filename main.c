#include "header.h"
int world_cord_x;
int world_cord_y;
int n_drones;
int S, Q, T;
Warehouse **warehouseArray;
ProductTypeList productType;
ProductList prodList;
int shmid; 
Stats *stats_ptr;
pthread_t drone_thread[4]; //Número de Drones
int power = 1;

int main(){
	signal(SIGINT, signal_handler);
    
    printf("oi");

    read_config();
    create_thread_pool();
    create_shared_memory();
    create_process();
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
 
    char *token;
    char line[255];
    fgets(line, sizeof(line), fp);
    char wc_x[50], wc_y[50];
    int n_warehouses;
    //ir buscar primeira ","
    token = strtok(line, ",");
    strcpy(wc_x, token);
    token = strtok(NULL, ",");
    if(token[0] == ' '){
        token++;
        }
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
        }
    strcpy(wc_y, token);
    world_cord_x = atoi(wc_x);
    world_cord_x = atoi(wc_y);
    char p_name[50];
    productType = create_product_type_list();
    fgets(line, sizeof(line), fp);
    //ir buscar primeira ","
    token = strtok(line, ",");
    while(token != NULL){
        while(token[0] == ' '){
            token++;
        }
        if(token[strlen(token)-1] == '\n'){
            token[strlen(token)-1] = '\0';
        }
        strcpy(p_name, token);
        insert_product_type(p_name, productType);
        token = strtok(NULL, ",");
    }
 
    fgets(line, sizeof(line), fp);
    token = strtok(line, "\n");
    n_drones = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(line, ",");
    S = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(NULL, ",");
    if(token[0] == ' '){
        token++;
    }
    Q = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(NULL, ",");
    if(token[0] == ' '){
        token++;
    }
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }
    T = atoi(token);
    token = strtok(line, "\n");
 
   
    n_warehouses = atoi(token);
 
    warehouseArray = malloc(n_warehouses * sizeof(Warehouse*));
    prodList = create_product_list();
    int quantity;
    //meter num for
    char prodName[50];
    fflush(stdin);
    for(int i=0; i<n_warehouses; i++){
        if(fgets(line, sizeof(line), fp) != NULL){
            token = strtok(line, " ");
            Warehouse *h = (Warehouse*) malloc(sizeof(Warehouse));
            strcpy(h->w_name, token);
            token = strtok(NULL, "; ");
            token = strtok(NULL, ",");
            h->w_x = atof(token);
            token = strtok(NULL, " ");
            h->w_y = atof(token);
            warehouseArray[i] = h;
            token = strtok(NULL, ": ");
            while(token != NULL){
                token = strtok(NULL, ", ");
                strcpy(prodName, token);
                token = strtok(NULL, ", ");
                quantity = atoi(token);
                insert_product(prodName, quantity, h->w_name, prodList);
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1] = '\0';
                    token = NULL;
                }
            }
 
        }
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
    printf("Inserted %s into the linked list\n", insertProduct->product_type.p_name);
}
 
void list_product_types(ProductTypeList productType){
    ProductTypeList node;
    node = productType->next;
    while(node != NULL){
        printf("Type: %s\n", node->product_type.p_name);
        node = node->next;
    }
}

//cria lista ligada de product types
ProductList create_product_list(void){
 
    ProductList prod_node;
    prod_node = (ProductList) malloc(sizeof(product_node));
 
    if(prod_node != NULL){
        prod_node->next = NULL;
    }
    printf("List created\n");
    return prod_node;
}
 
void insert_product(char p_name[50], int quantity, char w_name[50], ProductList prodList){
    ProductList insertProd;
    ProductList atual = prodList;
    insertProd = malloc(sizeof(product_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    strcpy(insertProd->product.p_name, p_name);
    insertProd->product.quantity = quantity;
    strcpy(insertProd->product.w_name, w_name);
    atual->next = insertProd;
    insertProd->next = NULL;
    printf("Inserted %s into the linked list\n", insertProd->product.p_name);
}
 
int check_prod_type(char p_name[50], ProductTypeList productType){
    ProductTypeList node;
    node = productType->next;
    while(node != NULL){
        if(strcmp(node->product_type.p_name, p_name) == 0)
            return 1;
        else{
            node = node->next;
        }
}
    return 0;
}
 
void list_product(ProductList product){
    ProductList node;
    node = product->next;
    while(node != NULL){
        printf("Prod: %s\n", node->product.p_name);
        printf("Quantity: %d\n", node->product.quantity);
        printf("W_Name: %s\n", node->product.w_name);
        node = node->next;
    }
}

void create_process(){
    
    pid_t pid;
    pid = fork();

    if(pid == 0){
        warehouse_activity();
    }
    else{
        central();
    }

}

void warehouse_activity(){
    printf("\n---Warehouse created---\n");
}

void central(){
    printf("\n---Central Working---");
}