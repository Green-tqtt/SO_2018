#include "header.h"
#include "drone_movement.c"
int shmid;
int fd; 
Stats *stats_ptr;
int power = 1;
pthread_t *drone_threads;
int *drone_id;
int retStat;
pid_t wpid;
int status = 0;
SearchResult *search_result;
sem_t *access_shared_mem;

//TEST DATA//
char type_test[50] = "Prod_D";
int quantity_test = 7;

//------SHARED MEMORY------//
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
    stats_ptr->n_drones = 0;
    stats_ptr->Q = 0;
    stats_ptr->S = 0;
    stats_ptr->T = 0;
    stats_ptr->world_cord_x = 0;
    stats_ptr->world_cord_y = 0;
    stats_ptr->n_warehouses = 0;
    stats_ptr->droneList = NULL;
    stats_ptr->prodType = NULL;
    stats_ptr->wArray = NULL;
 
    //Dados teste

    printf("----------RESULTS----------\n");
    printf("Número de encomendas atribuídas a drones: %d\n", stats_ptr->n_e_drones);
    printf("Número de produtos carregados nos armazéns: %d\n", stats_ptr->n_p_warehouse);
    printf("Número de encomendas entregues: %d\n", stats_ptr->n_e_delivered);
    printf("Número de produtos entregues: %d\n", stats_ptr->n_p_delivered);
    printf("Tempo médio para conclusão de uma encomenda: %.1f\n", stats_ptr->average_time);
    printf("---------------------------\n");
	
}
void init_sem(){
    sem_unlink("access_shared_mem");
    access_shared_mem = sem_open("access_shared_mem", O_CREAT | O_EXCL, 0700, 1);
}

void close_sem(){
    sem_unlink("access_shared_mem");
    sem_close(access_shared_mem);
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

/*void signal_handler(int signum){
    while(wait(NULL)>0);
    //shutdown_semaphores();
    //stats_results();
    power = 0;
    destroy_shared_memory();
    destroy_thread_pool();
    kill(0, SIGKILL);
    exit(0);
}*/

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
    stats_ptr->world_cord_x = atoi(wc_x);
    stats_ptr->world_cord_y = atoi(wc_y);
    char p_name[50];
    stats_ptr->prodType = create_product_type_list();
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
        insert_product_type(p_name, stats_ptr->prodType);
        token = strtok(NULL, ",");
    }
 
    fgets(line, sizeof(line), fp);
    token = strtok(line, "\n");
    stats_ptr->n_drones = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(line, ",");
    stats_ptr->S = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(NULL, ",");
    if(token[0] == ' '){
        token++;
    }
    stats_ptr->Q = atoi(token);
 
    fgets(line, sizeof(line), fp);
    token = strtok(NULL, ",");
    if(token[0] == ' '){
        token++;
    }
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }
    stats_ptr->T = atoi(token);
    token = strtok(line, "\n");
 
    n_warehouses = atoi(token);
    stats_ptr->n_warehouses = n_warehouses;
 
    stats_ptr->wArray= malloc(n_warehouses * sizeof(Warehouse*));
    int quantity;
    int checkProdType;
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
            stats_ptr->wArray[i] = h;
            stats_ptr->wArray[i]->prodList = create_product_list();
            token = strtok(NULL, ": ");
            while(token != NULL){
                token = strtok(NULL, ", ");
                strcpy(prodName, token);
                token = strtok(NULL, ", ");
                quantity = atoi(token);
                checkProdType = check_prod_type(prodName, stats_ptr->prodType);
                if(checkProdType == 0){
                    printf("%s is not initialized in config.txt, this may bring problems in the long run.\n", prodName);
                }
                insert_product(prodName, quantity, stats_ptr->wArray[i]->prodList);
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1] = '\0';
                    token = NULL;
                }
            }
 
        }
    }
    fclose(fp);
    //DEBUG
    /*printf("\t\tPROD TYPES\n");
    list_product_types(stats_ptr->prodType);
    printf("\t\tWAREHOUSE LIST\n");
    for(int i=0; i<n_warehouses; i++){
        printf("W_NAME: %s\n", stats_ptr->wArray[i]->w_name);
        printf("W_XY: %f\n", stats_ptr->wArray[i]->w_x);
        printf("W_NAME: %f\n", stats_ptr->wArray[i]->w_y);
        printf("\t\tPRODUCT LIST\n");
        list_product(stats_ptr->wArray[i]->prodList);
        printf("\n");
    }*/

}

void create_named_pipe(){
    if((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)){
        perror("Error creating named pipe: ");
        exit(0);
    }

    printf("Named pipe successfully created\n");

}

//------LIST STUFF------//
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
    ProductTypeList atual = productType;
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

//lista ligada de drones
DroneList create_drone_list(void){
 
    DroneList d_node;
    d_node = (DroneList) malloc(sizeof(drone_node));
 
    if(d_node != NULL){
        d_node->next = NULL;
    }
    printf("List created\n");
    return d_node;
}

void insert_drone(int drone_id, int d_x, int d_y, DroneList droneList){
    DroneList insertDrone;
    DroneList atual = droneList;
    insertDrone = malloc(sizeof(drone_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    insertDrone->drone.d_x = d_x;
    insertDrone->drone.d_y = d_y;
    insertDrone->drone.state = 0;
    insertDrone->drone.drone_id = drone_id;
    atual->next = insertDrone;
    insertDrone->next = NULL;
    printf("Inserted %d into the linked list\n", insertDrone->drone.drone_id);
}
 
void insert_product(char p_name[50], int quantity, ProductList prodList){
    ProductList insertProd;
    ProductList atual = prodList;
    insertProd = malloc(sizeof(product_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    strcpy(insertProd->product.p_name, p_name);
    insertProd->product.quantity = quantity;
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
        node = node->next;
    }
}

void list_drones(DroneList drone){
    DroneList node;
    node = drone->next;
    while(node != NULL){
        printf("Drone: %d\n", node->drone.drone_id);
        node = node->next;
    }
}

//------WAREHOUSE PROCESS-----//
void warehouse_handler(int i){
    sem_wait(access_shared_mem);
    stats_ptr->wArray[i]->w_no = getpid();
    sem_post(access_shared_mem);
    printf("[%d] Hello! I'm a warehouse!\n", getpid());
    /*printf("W_NAME: %s\n", stats_ptr->wArray[i]->w_name);
    printf("W_XY: %f\n", stats_ptr->wArray[i]->w_x);
    printf("W_NAME: %f\n", stats_ptr->wArray[i]->w_y);
    printf("W_NO: %d\n", stats_ptr->wArray[i]->w_no);
    printf("\t\tPRODUCT LIST\n");
    list_product(stats_ptr->wArray[i]->prodList);
    printf("\n");*/
    sleep(20);
    printf("[%d] Goodbye!\n", getpid());
    exit(0);
}

void warehouse(){
    pid_t pid = getpid();
    int forkVal;
    int n_warehouses;
    n_warehouses = stats_ptr->n_warehouses;
    for(int i=0; i<n_warehouses; i++){
        forkVal = fork();
        if(forkVal == 0){
            pid = getpid();
            printf("[%d] Warehouse %d is active\n", getpid(), i);
            stats_ptr->wArray[i]->w_no = getpid();
            //chama função worker
            warehouse_handler(i);
        }
        else if (forkVal < 0){
        perror("Error creating process\n");
        exit(0);
        }
    }
}

void update_order_drones(){
    sem_wait(access_shared_mem);
    stats_ptr->n_e_drones++;
    sem_post(access_shared_mem);
}

int update_stock(char prod_name[50], int quantity, char w_name[50], int id){
    Stats *aux_node = stats_ptr;
    int n_warehouses = stats_ptr->n_warehouses;
    char prod_cmp[50];
    char w_cmp[50];
    for(int i=0; i<n_warehouses; i++){
        strcpy(w_cmp, aux_node->wArray[i]->w_name);
        if(strcmp(w_cmp, w_name) == 0){
            ProductList aux_prod = aux_node->wArray[i]->prodList->next;
            while(aux_prod != NULL){
                strcpy(prod_cmp, aux_prod->product.p_name);
                if(strcmp(prod_cmp, prod_name) == 0){
                    int new_quantity;
                    new_quantity = aux_prod->product.quantity - quantity;
                    aux_prod->product.quantity = new_quantity;
                    return 1;
                }
                aux_prod = aux_prod->next;
            }
        }
    }
    return 0;
}

//------CENTRAL PROCESS-----//
void *drone_handler(void *id){
    int i = *(int*)id;
    int inside_id= -1;
    int sleep_count=0;
    Drone myDrone;
    SearchResult *handler_check = NULL;
    DroneList aux_node = stats_ptr->droneList->next;
    while(inside_id != i){
        inside_id = aux_node->drone.drone_id;
        if(inside_id == i){
            myDrone = aux_node->drone;
        }
        aux_node = aux_node->next;
    }


    printf("[%d] Awaiting orders... \n", myDrone.drone_id);
    while(myDrone.state == 0){
        handler_check = search_result;
        if(handler_check->drone_id == myDrone.drone_id){
            printf("[%d] An order has arrived for me!\n", myDrone.drone_id);
            update_order_drones();
            myDrone.state = 1;
        }
        sleep(1);
        sleep_count+=1;
        if(sleep_count == 10){
            break;
        }
    }
    if(myDrone.state == 1){
        int move = move_towards(&myDrone.d_x, &myDrone.d_y, handler_check->w_x, handler_check->w_y);
        printf("[%d] I'm moving torwards %s...\n", myDrone.drone_id, handler_check->w_name);
        if(move == 1){
            int check;
            printf("[%d] I reached %s!\n", myDrone.drone_id, handler_check->w_name);
            sem_wait(access_shared_mem);
            check = update_stock(type_test, quantity_test, search_result->w_name, i);
            sem_post(access_shared_mem);
            if(check == 1){
                printf("[%d] Updated stock number for you!\n", i);
            }
            else{
                printf("[%d] Stock number was not updated\n", i);
            }
        }
        else{
            printf("[%d] I'm sorry, I couldn't reach the destination\n", myDrone.drone_id);
        }
    }
    else
        printf("[%d] No orders for me, leaving\n", myDrone.drone_id);
    pthread_exit(NULL);
}

void kill_threads(){
    int n_drones = stats_ptr->n_drones;
    for(int i=0; i<n_drones; i++){
        if(pthread_join(drone_threads[i], NULL)==0){
            printf("[%d] Drone thread has finished\n", drone_id[i]);
        }
        else{
            perror("Error killing Drone thread\n");
        }
    }
}

void drones_init(){
    time_t t;
    srand((unsigned) time(&t));
    stats_ptr->droneList = create_drone_list();
    int random_num;
    int base_x, base_y, drone_id;
    for(int i=0; i<stats_ptr->n_drones; i++){
        random_num = 1 + rand() % 4;
        drone_id = i;
        if(i == 1){
            base_x = 0;
            base_y = stats_ptr->world_cord_y;
        }
        else if(i == 2){
            base_x = stats_ptr->world_cord_x;
            base_y = stats_ptr->world_cord_y;
        }
        else if(i == 3){
            base_x = 0;
            base_y = 0;
        }
        else{
            base_x = stats_ptr->world_cord_x;
            base_y = 0;
        }
        insert_drone(drone_id, base_x, base_y, stats_ptr->droneList);
    }

}

SearchResult choose_drone(){
    //sem
    double dist_result = 0;
    double prev_dist = 0;
    int result_id = 0;
    DroneList aux_drone_node = stats_ptr->droneList->next;
    DroneList aux_first_node = stats_ptr->droneList->next;
    Stats *aux_stats_ptr = stats_ptr;
    for(int i=0; i<stats_ptr->n_warehouses; i++){
        ProductList aux_prod_node = stats_ptr->wArray[i]->prodList->next;
        while(aux_prod_node!= NULL){
            if(strcmp(aux_prod_node->product.p_name, type_test) == 0 && aux_prod_node->product.quantity >= quantity_test){
                printf("\t\tPRODUCT %s IS AVAILABLE AT %s\n", type_test, aux_stats_ptr->wArray[i]->w_name);
                printf("Calculating distance...\n");
                //alguma cena é um next aqui
                while(aux_drone_node != NULL){
                    if(aux_drone_node->drone.state == 0){
                        dist_result = distance(aux_drone_node->drone.d_x, aux_drone_node->drone.d_x, aux_stats_ptr->wArray[i]->w_x, aux_stats_ptr->wArray[i]->w_y);
                        printf("Distance: %f\n", dist_result);
                        printf("Drone: %d\n", aux_drone_node->drone.drone_id);
                        printf("\n");
                        if(prev_dist == 0 || prev_dist > dist_result){
                            prev_dist = dist_result;
                            result_id = aux_drone_node->drone.drone_id;
                            search_result->drone_id = result_id;
                            strcpy(search_result->w_name, aux_stats_ptr->wArray[i]->w_name);
                            search_result->w_x = aux_stats_ptr->wArray[i]->w_x;
                            search_result->w_y = aux_stats_ptr->wArray[i]->w_y;
                            search_result->distance = dist_result;
                        }
                    }
                    aux_drone_node = aux_drone_node->next;
                }
                aux_drone_node = aux_first_node;
            }
            aux_prod_node = aux_prod_node->next;
        }
    }
    return *search_result;
}
void central(){
	int i;
    int n_drones = stats_ptr->n_drones;
    drones_init();
    create_named_pipe();
    drone_threads = malloc(sizeof(pthread_t)*stats_ptr->n_drones);
    drone_id = malloc(sizeof(int)*stats_ptr->n_drones);
    search_result = malloc(sizeof(SearchResult));
    search_result->drone_id = -1;
    for(i = 0; i < n_drones; i++){
        drone_id[i] = i;
        if(pthread_create(&drone_threads[i], NULL, drone_handler, &drone_id[i])==0){
            printf("[%d] Drone is active\n", drone_id[i]);
        }
        else{
            perror("Error creating Drone thread\n");
        }
    }
        *search_result = choose_drone();

}
void unlink_named_pipe(){
    int unlk;
    unlk = unlink(PIPE_NAME);
    if(unlk == 0){
        printf("Named Pipe unlinked!\n");
    }
    
}

int main(){
	/*signal(SIGINT, signal_handler);
    printf("oi");
    read_config();
    while(power){
    }*/
    printf("Simulation manager started\n");
    create_shared_memory();
    read_config();

    pid_t pid = getpid();
    printf("SM PID: %d\n", pid);
    int forkVal = fork();
    if(forkVal == 0){
        //child, chamar funcao que cria threads
        central();
        kill_threads();
    }
    else if (forkVal < 0){
        perror("Error creating process\n");
        exit(0);
    }
    else{
        //parent process, chamar processo warehouse
        warehouse();
        //waits to child process
        for(int i=0; i<stats_ptr->n_warehouses + 1; i++){
            wait(NULL);
        }
        destroy_shared_memory();
        close_sem();
        unlink_named_pipe();
    }
}