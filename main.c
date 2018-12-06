#include "header.h"
#include "drone_movement.c"
ProductTypeList prodType;
int shmid;
int w_shmid;
int fd; 
Stats *stats_ptr;
pthread_t *drone_threads;
int *drone_id;
int retStat;
pid_t wpid;
int status = 0;
SearchResult *search_result;
sem_t *access_shared_mem;
sem_t *control_file_write;
FILE *log_file;
DroneList droneList;

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
    printf("[SM]Sucessfully shmdt'd\n");
    if(shmctl(shmid, IPC_RMID, NULL) == -1){
        perror("Error unmapping shared memory\n");
    }
    printf("[SM]Sucessfully shmctl'd\n");
}

void destroy_shared_memory_warehouse(){
    if(shmdt(w_ptr) == -1){
        perror("Error using shmdt\n");
    }
    printf("[Warehouse]Sucessfully shmdt'd\n");
    if(shmctl(w_shmid, IPC_RMID, NULL) == -1){
        perror("Error unmapping shared memory\n");
    }
    printf("[Warehouse]Sucessfully shmctl'd\n");
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
    insertDrone->drone.dronePackage = NULL;
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

PackageList create_package_list(void){
    PackageList package_node;
    package_node = (PackageList) malloc(sizeof(package_node));
 
    if(package_node != NULL){
        package_node->next = NULL;
    }
    printf("List created\n");
    return package_node;
}

void insert_package(int uid, char prod_type[100], int quantity, int deliver_y, int deliver_x, PackageList packageList){
    PackageList insertPack;
    PackageList atual = packageList;
    insertPack = malloc(sizeof(package_node));
    while(atual->next!= NULL){
        atual = atual->next;
    }
    insertPack->package.uid = uid;
    strcpy(insertPack->package.prod_type, prod_type);
    insertPack->package.quantity = quantity;
    insertPack->package.deliver_y = deliver_y;
    insertPack->package.deliver_x = deliver_x;
    atual->next = insertPack;
    insertPack->next = NULL;

    printf("Inserted %d into the linked list\n", insertPack->package.uid);
}

void list_packages(PackageList packageList){
    PackageList node;
    node = packageList->next;
    while(node != NULL){
        printf("Package UID: %d\n", node->package.uid);
        printf("Prod_Type: %s\n", node->package.prod_type);
        printf("Quantity: %d\n", node->package.quantity);
        printf("D_X: %f\n", node->package.deliver_x);
        printf("D_Y: %f\n", node->package.deliver_y);
        printf("\n");
        node = node->next;
    }
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
            printf("[%d] Warehouse %d is active\n", getpid(), i);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Created Warehouse %d with PID: %d\n", hour, minutes, seconds, i, getpid());
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
    int state=0;
    int base_x, base_y, drone_id;
    int j=0;
    for(int i=0; i<n_drones; i++){
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
        insert_drone(drone_id, state, base_x, base_y, droneList);
    }

}

void central_exit(int signum){
    exit_flag=1;
    kill_threads();
    exit(0);
}

void create_new_threads(){
    exit_flag = 1;
    kill_threads();
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
    PackageList packageList;
    //----
    int n_drones = stats_ptr->n_drones;
    packageList = create_package_list();
    droneList = create_drone_list();
    drones_init(droneList, n_drones);
    create_named_pipe();
    create_threads(n_drones);
    read_pipe(packageList, droneList);
    
}

void create_threads(int n_drones){
       //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;
    
    drone_threads = malloc(sizeof(pthread_t)*stats_ptr->n_drones);
    drone_id = malloc(sizeof(int)*stats_ptr->n_drones);
    for(int i = 0; i < n_drones; i++){
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
    destroy_shared_memory();
    destroy_shared_memory_warehouse();
    close_sem();
    unlink_named_pipe();
    close_file();
    exit(0);
}

void read_pipe(PackageList packageList, DroneList droneList){

    //opens pipe for reading
    if((fd = open(PIPE_NAME, O_RDWR))< 0){
        perror("Error opening pipe for reading: ");
        exit(0);
    }

    char buffer[MAX];
    int bufferlen;
    char *token;
    int i=0;
    int result = -1;
    char prod[100] = "Prod_";
    Package order;

    while(1){
        if((bufferlen = read(fd, buffer, MAX))>0){
            if(buffer[bufferlen-1] == '\n') buffer[bufferlen-1] = '\0';

            token = strtok(buffer, " ");

            if(token != NULL && strcmp(token, "ORDER") == 0){
                token = strtok(NULL, " ");
                //nome da encomenda, adicionar a lista ligada de encomendas
                if(token != NULL){
                    token = strtok(NULL, ":");
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        //produto da encomenda aqui
                        char prod_name = token[strlen(token)-2];
                        strncat(prod, &prod_name, 50);
                        //prod number aqui
                        token = strtok(NULL, " ");
                        if(token != NULL && atoi(token) != 0){
                            int prod_number = atoi(token);
                            token = strtok(NULL, " ");
                            token = strtok(NULL, ", ");
                            if(token != NULL && atoi(token) != 0){
                                //cord x to deliver
                                int x_deliver = atoi(token);
                                token = strtok(NULL, " ");
                                if(token != NULL && atoi(token) != 0){
                                    //cord y to deliver
                                    int y_deliver = atoi(token);
                                    if(strtok(NULL, " ") != NULL){
                                        printf("\nInvalid command\n");
                                    }
                                    else{
                                        i++;
                                        result = goto_closest_warehouse(prod, prod_number);
                                        printf("Result of search is: %d\n", result);
                                        if(result == -1){
                                            insert_package(i, prod, prod_number, y_deliver, x_deliver, packageList);
                                            list_packages(packageList);
                                        }
                                        else{
                                            printf("Product is available!\n");
                                        }
                                    }
                                }
                                else{
                                    printf("\tInvalid command\n");
                                }
                            }
                            else{
                                printf("\tInvalid command\n");
                            }
                        }
                        else{
                            printf("\tInvalid command\n");
                        }
                    }
                    else{
                        printf("\tInvalid command\n");
                    }
                }
                else{
                    printf("\tInvalid command\n");
                }
            }
            else if(token != NULL && strcmp(token, "DRONE") == 0){
                token = strtok(NULL, " ");
                if(token != NULL && strcmp(token, "SET") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL && atoi(token) != 0){
                        int number_drones = atoi(token);
                        printf("The number of drones to change is: %d\n", number_drones);
                        if(strtok(NULL, " ") != NULL){
                            printf("\tInvalid command\n");
                        }
                        else{
                            create_new_threads();
                            sleep(5);
                            stats_ptr->n_drones = number_drones;
                            create_threads(number_drones);
                        }
                    }
                    else{
                        printf("\tInvalid command\n");
                    }
                }
                else{
                    printf("\tInvalid command\n");
                }
            }
            else{
                printf("\tInvalid command\n");
            }
            fflush(stdout);
        }
        prod[0] = '\0';
        strcpy(prod, "Prod_");
    }
}

int goto_closest_warehouse(char type[50], int quantity){
    //Usar Variaveis teste por agora *_test
    Warehouse *aux_w = w_ptr;
    DroneList aux_d = droneList;
    int warehouse_n = -1;
    double distD_W;
    int drone_id;
    double distMin = 999999;
    int n_warehouses = stats_ptr->n_warehouses;
    for( int i = 0; i < n_warehouses; i ++){
        for(int j = 0; j<3; j++){
            printf("p_name: %s\n", aux_w[i].prodList[j].p_name);
            printf("prod do read: %s\n", type);
            if(strcmp(aux_w[i].prodList[j].p_name, type) == 0){
                if (aux_w[i].prodList[j].quantity > quantity){
                    printf("\nWarehouse found! New warehouse is : %d", i);
                    printf("venho aqui, quant maior\n");
                    warehouse_n = i;
                }
            }
        }
    }
    if(warehouse_n == -1){
        return warehouse_n;
    }
    else{
        while(aux_d){
            distD_W = distance(aux_d->drone.d_x, aux_d->drone.d_y, aux_w[warehouse_n].w_x, aux_w[warehouse_n].w_y);
            printf("\n\tDist: %f, Drone: %d\n", distD_W, aux_d->drone.drone_id);
            if(distD_W < distMin){
                distMin = distD_W;
                drone_id = aux_d->drone.drone_id;
            }
            aux_d = aux_d->next;
        }
        printf("\nWarehouse mais proxima da Warehouse %d ---> Drone %d\n", drone_id, warehouse_n);
    }
    return 0;
}

int main(){
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