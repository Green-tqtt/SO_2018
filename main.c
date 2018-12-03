#include "header.h"
#include "drone_movement.c"
ProductTypeList prodType;
int shmid;
int w_shmid;
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
sem_t *control_file_write;
FILE *log_file;

int *warehouse_pids;

//TEST DATA//
char type_test[50] = "Prod_D";
int quantity_test = 7;
int exit_flag=0;

//------SHARED MEMORY------//
void create_shared_memory(){
    if((shmid = shmget(IPC_PRIVATE, sizeof(Stats), IPC_CREAT |0766)) == -1){
        perror("Error creating shared memory\n");   
        exit(1);
    }
    stats_ptr = (Stats*) shmat(shmid, NULL, 0);
    printf("----------SHARED MEMORY----------\n");
    printf("Shared memory sucessfully at address %p\n", stats_ptr);
    printf("Shared memory at %d\n", shmid);
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
 
    //Dados teste

    printf("----------RESULTS----------\n");
    printf("Número de encomendas atribuídas a drones: %d\n", stats_ptr->n_e_drones);
    printf("Número de produtos carregados nos armazéns: %d\n", stats_ptr->n_p_warehouse);
    printf("Número de encomendas entregues: %d\n", stats_ptr->n_e_delivered);
    printf("Número de produtos entregues: %d\n", stats_ptr->n_p_delivered);
    printf("Tempo médio para conclusão de uma encomenda: %.1f\n", stats_ptr->average_time);
    printf("---------------------------\n");
	
}

void create_warehouse_sm(int n_warehouses){
    if((w_shmid = shmget(IPC_PRIVATE, sizeof(Warehouse)*n_warehouses , IPC_CREAT |0766)) == -1){
        perror("Error creating shared memory\n");   
        exit(1);
    }
    w_ptr = (Warehouse*) shmat(w_shmid, NULL, 0);
}

void init_sem(){
    sem_unlink("access_shared_mem");
    sem_unlink("control_file_write");
    access_shared_mem = sem_open("access_shared_mem", O_CREAT | O_EXCL, 0700, 1);
    control_file_write = sem_open("control_file_write", O_CREAT | O_EXCL, 0700, 1);
}

void close_sem(){
    sem_unlink("access_shared_mem");
    sem_unlink("control_file_write");
    sem_close(access_shared_mem);
    sem_close(control_file_write);
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

void destroy_shared_memory_warehouse(){
    if(shmdt(w_ptr) == -1){
        perror("Error using shmdt\n");
    }
    printf("Sucessfully shmdt'd\n");
    if(shmctl(w_shmid, IPC_RMID, NULL) == -1){
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
    stats_ptr->world_cord_x = atoi(wc_x);
    stats_ptr->world_cord_y = atoi(wc_y);
    char p_name[50];
    prodType = create_product_type_list();
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
        insert_product_type(p_name, prodType);
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
    
    int quantity;
    int checkProdType;
    //meter num for
    char prodName[50];
    fflush(stdin);
    create_warehouse_sm(n_warehouses);
    int j=0;
    for(int i=0; i<n_warehouses; i++){
        if(fgets(line, sizeof(line), fp) != NULL){
            Warehouse wh;
            token = strtok(line, " ");
            strcpy(wh.w_name, token);
            token = strtok(NULL, "; ");
            token = strtok(NULL, ",");
            wh.w_x = atof(token);
            token = strtok(NULL, " ");
            wh.w_y = atof(token);
            token = strtok(NULL, ": ");
            wh.w_no = i+1;
            while(token != NULL){
                token = strtok(NULL, ", ");
                strcpy(prodName, token);
                token = strtok(NULL, ", ");
                quantity = atoi(token);
                checkProdType = check_prod_type(prodName, prodType);
                if(checkProdType == 0){
                    printf("%s is not initialized in config.txt, this may bring problems in the long run.\n", prodName);
                }
                strcpy(wh.prodList[j].p_name, prodName);
                wh.prodList[j].quantity = quantity;
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1] = '\0';
                    token = NULL;
                }
                w_ptr[i] = wh;
                j++;
            }
            j=0;
        }
    }
    warehouse_pids = malloc(sizeof(int)*n_warehouses);
    fclose(fp);
    //DEBUG
    /*for(int i=0; i<n_warehouses; i++){
        printf("Warehouse Name: %s\n", w_ptr[i].w_name);
        printf("W_X: %f\n", w_ptr[i].w_x);
        printf("W_Y: %f\n", w_ptr[i].w_y);
        printf("W_NO: %d\n", w_ptr[i].w_no);
        printf("\t%s Products:\n", w_ptr[i].w_name);
        for(int j=0; j<3; j++){
            printf("\tProd: %s\n", w_ptr[i].prodList[j].p_name);
            printf("\tQuant: %d\n", w_ptr[i].prodList[j].quantity);
        }
        printf("\n");
    }*/

}

void open_log_file(){
    log_file = fopen("logs.log", "a+");
    if(log_file == NULL){
        perror("Couldn't create file");
    }
}

void close_file(){
    fclose(log_file);
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

//criar lista ligada de drones
DroneList create_drone_list(void){
 
    DroneList drone_node;
    drone_node = (DroneList) malloc(sizeof(drone_node));
 
    if(drone_node != NULL){
        drone_node->next = NULL;
    }
    printf("List created\n");
    return drone_node;
}

void insert_drone(int drone_id, int state, double d_x, double d_y, DroneList droneList){
    DroneList insertDrone;
    DroneList atual = droneList;
    insertDrone = malloc(sizeof(drone_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    insertDrone->drone.drone_id = drone_id;
    insertDrone->drone.state = state;
    insertDrone->drone.d_x = d_x;
    insertDrone->drone.d_y = d_y;
    atual->next = insertDrone;
    insertDrone->next = NULL;
    printf("Inserted %d into the linked list\n", insertDrone->drone.drone_id);
}

void list_drones(DroneList droneList){
    DroneList node;
    node = droneList->next;
    while(node != NULL){
        printf("Drone ID: %d\n", node->drone.drone_id);
        printf("State: %d\n", node->drone.state);
        printf("D_x: %f\n", node->drone.d_x);
        printf("D_y: %f\n", node->drone.d_y);
        printf("\n");
        node = node->next;
    }
}

void processes_exit(){
    printf("[%d] Goodbye!\n", getpid());
    exit(0);
}

//------WAREHOUSE PROCESS-----//
void warehouse_handler(int i){
    signal(SIGINT, processes_exit);
    printf("[%d] Hello! I'm a warehouse!\n", getpid());
    /*printf("W_NAME: %s\n", stats_ptr->wArray[i]->w_name);
    printf("W_XY: %f\n", stats_ptr->wArray[i]->w_x);
    printf("W_NAME: %f\n", stats_ptr->wArray[i]->w_y);
    printf("W_NO: %d\n", stats_ptr->wArray[i]->w_no);
    printf("\t\tPRODUCT LIST\n");
    list_product(stats_ptr->wArray[i]->prodList);
    printf("\n");*/
    while(1){
        if(exit_flag == 1){
            break;
        }
    }
    printf("[%d] Goodbye!\n", getpid());
    exit(0);
}

void warehouse(){
    int forkVal;
    int n_warehouses;
    n_warehouses = stats_ptr->n_warehouses;

    //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;
    
    for(int i=0; i<n_warehouses; i++){
        forkVal = fork();
        if(forkVal == 0){
            warehouse_pids[i] = getpid();
            printf("[%d] Warehouse %d is active\n", getpid(), i);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Created Warehouse %d with PID: %d\n", hour, minutes, seconds, i, warehouse_pids[i]);
            sem_post(control_file_write);
            //chama função worker
            warehouse_handler(i);
        }
        else if (forkVal < 0){
            perror("Error creating process\n");
            exit(1);
        }
    }
}

void update_order_drones(){
    sem_wait(access_shared_mem);
    stats_ptr->n_e_drones++;
    sem_post(access_shared_mem);
}

//------CENTRAL PROCESS-----//
void *drone_handler(void *id){
    int i = *(int*)id;
    printf("jshdkjs\n");
    //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;
    while(1){
        printf("[%d] I'm working!\n", i);
        if(exit_flag == 1){
            break;
        }
        sleep(5);
    }
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
    free(drone_threads);
    free(drone_id);
}

void drones_init(DroneList droneList, int n_drones){
    time_t t;
    srand((unsigned) time(&t));
    int random_num;
    int state=0;
    int base_x, base_y, drone_id;
    int j=0;
    for(int i=0; i<n_drones; i++){
        random_num = 1 + rand() % 4;
        drone_id = i;
        if(j == 0){
            base_x = 0;
            base_y = stats_ptr->world_cord_y;
            j++;
            if(j==4) j=0;
        }
        else if(j == 1){
            base_x = stats_ptr->world_cord_x;
            base_y = stats_ptr->world_cord_y;
            j++;
            if(j==4) j=0;
        }
        else if(j == 3){
            base_x = 0;
            base_y = 0;
            j++;
            if(j==4) j=0;
        }
        else{
            base_x = stats_ptr->world_cord_x;
            base_y = 0;
            j++;
            if(j==4) j=0;
        }
        //inserir drones no array
        insert_drone(drone_id, state, base_x, base_y, droneList);
    }
    list_drones(droneList);

}

void central_exit(int signum){
    printf("Central exit\n");
    exit_flag=1;
    kill_threads();
    printf("threads exit\n");

    exit(0);
}


void central(){
    //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;
    DroneList droneList;

    //----
	int i;
    int n_drones = stats_ptr->n_drones;
    droneList = create_drone_list();
    drones_init(droneList, n_drones);
    create_named_pipe();
    drone_threads = malloc(sizeof(pthread_t)*stats_ptr->n_drones);
    drone_id = malloc(sizeof(int)*stats_ptr->n_drones);
    //search_result = malloc(sizeof(SearchResult));
    //search_result->drone_id = -1;
    for(i = 0; i < n_drones; i++){
        drone_id[i] = i;
        if(pthread_create(&drone_threads[i], NULL, drone_handler, &drone_id[i])==0){
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Created Drone %d\n", hour, minutes, seconds, i);
            sem_post(control_file_write);
            printf("[%d] Drone is active\n", drone_id[i]);
        }
        else{
            perror("Error creating Drone thread\n");
        }
    }
        //*search_result = choose_drone();
    
}

void unlink_named_pipe(){
    int unlk;
    unlk = unlink(PIPE_NAME);
    if(unlk == 0){
        printf("Named Pipe unlinked!\n");
    }
    
}

void signal_handler(int signum){
    exit_flag=1;
    while(wait(NULL)>0);
    //kill_threads();
    destroy_shared_memory();
    destroy_shared_memory_warehouse();
    close_sem();
    unlink_named_pipe();
    close_file();
    exit(0);
}
int main(){
	/*signal(SIGINT, signal_handler);
    printf("oi");
    read_config();
    while(power){
    }*/
    signal(SIGINT, signal_handler);
    printf("Simulation manager started\n");
    create_shared_memory();
    read_config();
    open_log_file();
    pid_t pid = getpid();
    printf("SM PID: %d\n", pid);
    int forkVal = fork();
    if(forkVal == 0){
        //child, chamar funcao que cria threads
        signal(SIGINT, central_exit);
        central();
        while(1);
    }
    else if (forkVal < 0){
        perror("Error creating process\n");
        exit(0);
    }
    else{
        //parent process, chamar processo warehouse
        warehouse();
        //waits to child process
    }
    while(1);
    return 0;
}