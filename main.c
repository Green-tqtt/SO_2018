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
int mq_id;
//THREADS
pthread_cond_t drone_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t d_mutex = PTHREAD_MUTEX_INITIALIZER;
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
    stats_ptr->T = 0.0;
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

void create_message_queue(){

    if((mq_id = msgget(IPC_PRIVATE, IPC_CREAT|0700)) < 0){
        perror("Problem creating message queue");
        exit(0);
    }
    printf("Message queue created!\n");
}

void cleanup_mq(){
    if(msgctl(mq_id, IPC_RMID, NULL) < 0){
        perror("Error deleting message queue");
        exit(0);
    }
    printf("Message queue deleted!\n");
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
    stats_ptr->T = atof(token);
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
            wh.w_no = i;
            wh.state = 0;
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
    insertDrone->drone.origin_x = d_x;
    insertDrone->drone.origin_y = d_y;
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
        if(node->drone.dronePackage != NULL){
            printf("\tProd_type: %s\n", node->drone.dronePackage->prod_type);
            printf("\tQuantity: %d\n", node->drone.dronePackage->quantity);
            printf("\tDeliver_x: %f\n", node->drone.dronePackage->deliver_x);
            printf("\tDeliver_y: %f\n", node->drone.dronePackage->deliver_y);
            printf("\tOrder UID: %d\n", node->drone.dronePackage->uid);
            printf("\tOrder W_NO: %d\n", node->drone.dronePackage->w_no);
        }
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
    insertPack->package.w_no = -1;
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
        printf("Warehouse: %d\n", node->package.w_no);
        printf("\n");
        node = node->next;
    }
}

//------WAREHOUSE PROCESS-----//
void warehouse_handler(int i){
    signal(SIGINT, processes_exit);
    printf("[%d] Hello! I'm a warehouse!\n", getpid());
    Warehouse *aux_ptr = w_ptr;
    while(1){
        if(exit_flag == 1){
            break;
        }
        if(aux_ptr[i-1].state == 1){
            msg msg_rcv;
            msgrcv(mq_id, &msg_rcv, sizeof(msg)-sizeof(long), i, 0);
            printf("[%d] Warehouse %d received notification from Drone %d\n", getpid(), i, msg_rcv.drone_id);
            printf("[%d] Order details: %s: %d\n", getpid(), msg_rcv.prod_type, msg_rcv.quantity);
            printf("[%d] Current stock:\n", getpid());
            for(int j=0; j<3; j++){
                printf("[%d] %s: %d\n", getpid(), aux_ptr[i-1].prodList[j].p_name, aux_ptr[i-1].prodList[j].quantity);
            }
            printf("[%d] Sending confirmation to Drone %d\n", getpid(), msg_rcv.drone_id);
            msg msg_snd;
            msg_snd.mtype = msg_rcv.replyTo;
            msg_snd.drone_id = i;
            strcpy(msg_snd.prod_type, "NONE");
            msg_snd.quantity = 0;
            msg_snd.replyTo = 0;
            msgsnd(mq_id, &msg_snd, sizeof(msg)-sizeof(long), 0);
            aux_ptr[i-1].state = 0;
        }
        if(aux_ptr[i-1].state == 2){
            msg msg_supply;
            int msg_type = aux_ptr[i-1].w_no + 100;
            msgrcv(mq_id, &msg_supply, sizeof(msg)-sizeof(long), msg_type, 0);
            printf("[%d] Got a supply of %s: %d, updated stock!\n", getpid(), msg_supply.prod_type, msg_supply.quantity);
            printf("[%d] Current stock:\n", getpid());
            for(int j=0; j<3; j++){
                printf("[%d] %s: %d\n", getpid(), aux_ptr[i-1].prodList[j].p_name, aux_ptr[i-1].prodList[j].quantity);
            }
            aux_ptr[i-1].state = 0;
        }

    }
    printf("[%d] Goodbye!\n", getpid());
    exit(0);
}

void supply_warehouses(int j){
    Warehouse *aux_ptr = w_ptr;
    int random_val = rand() % 3;
    int quantity = 10;
    aux_ptr[j].prodList[random_val].quantity += quantity;
    printf("SELECTED WAREHOUSE: %s\n", aux_ptr[j].w_name);
    msg supply_msg;
    supply_msg.drone_id = 0;
    supply_msg.mtype = 100 + aux_ptr[j].w_no;
    strcpy(supply_msg.prod_type, aux_ptr[j].prodList[random_val].p_name);
    supply_msg.quantity = quantity;
    aux_ptr[j].state = 2;
    msgsnd(mq_id, &supply_msg, sizeof(supply_msg)-sizeof(long), 0);
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
            printf("[%d] Warehouse %d is active\n", getpid(), i+1);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Created Warehouse %d with PID: %d\n", hour, minutes, seconds, i, getpid());
            sem_post(control_file_write);
            //chama função worker
            warehouse_handler(i+1);
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
    DroneList  myNode;
    Warehouse *aux_ptr = w_ptr;
    myNode = find_drone_node(i, droneList);
    int move_warehouse = -3;
    int move_deliver = -3;
    int move_base = -3;
    if(myNode == NULL){
        printf("Something went incredibly wrong\n");
    }
    printf("[%d]This is my node: %d\n", i, myNode->drone.drone_id);

    while(1){
        pthread_mutex_lock(&d_mutex);
        int w_x = 0;
        int w_y = 0;
        double sleep_val = stats_ptr->T;
        while(exit_flag == 0 && myNode->drone.dronePackage == NULL){
            pthread_cond_wait(&drone_cond, &d_mutex);
        }
        if(exit_flag == 1){
            pthread_mutex_unlock(&d_mutex);
            break;
        }
        printf("[%d] I'm ready for a delivery! %s to x:%d, y:%d\n", i, myNode->drone.dronePackage->prod_type, (int)myNode->drone.dronePackage->deliver_x, (int)myNode->drone.dronePackage->deliver_y);
        printf("[%d] Moving to Warehouse %d...\n", i, myNode->drone.dronePackage->w_no);
        int n_warehouses = stats_ptr->n_warehouses;
        for(int i=0; i<n_warehouses; i++){
            if(aux_ptr[i].w_no == myNode->drone.dronePackage->w_no){
                w_x = aux_ptr[i].w_x;
                w_y = aux_ptr[i].w_y;
            }
        }
        while(move_warehouse != 0){
            move_warehouse = move_towards(&myNode->drone.d_x, &myNode->drone.d_y, w_x, w_y);

        }
        if(move_warehouse == 0){
            printf("[%d] Reached warehouse %d, %d!\n", i, (int)myNode->drone.d_x, (int)myNode->drone.d_y);
            myNode->drone.state = 3;
            printf("[%d] Notifying Warehouse...\n", i);
            msg msg_wh;
            msg_wh.mtype = myNode->drone.dronePackage->w_no;
            strcpy(msg_wh.prod_type, myNode->drone.dronePackage->prod_type);
            msg_wh.quantity = myNode->drone.dronePackage->quantity;
            msg_wh.drone_id = myNode->drone.drone_id;
            msg_wh.replyTo = myNode->drone.dronePackage->uid;
            msgsnd(mq_id, &msg_wh, sizeof(msg_wh)-sizeof(long), 0);

            msg msg_rcv;
            int type = myNode->drone.dronePackage->uid;
            msgrcv(mq_id, &msg_rcv, sizeof(msg)-sizeof(long), type, 0);
            printf("[%d] Supply received from Warehouse %d\n", i, msg_rcv.drone_id);
            myNode->drone.state = 4;
            move_warehouse = -3;

        }
        int order_x = myNode->drone.dronePackage->deliver_x;
        int order_y = myNode->drone.dronePackage->deliver_y;
        while(move_deliver != 0){
            move_deliver = move_towards(&myNode->drone.d_x, &myNode->drone.d_y, order_x, order_y);
            sleep(sleep_val);
            
        }
        if(move_deliver == 0){
            printf("[%d] Reached destination %d, %d!\n", i, (int)myNode->drone.d_x, (int)myNode->drone.d_y);
            move_deliver = -3;
            myNode->drone.dronePackage = NULL;
            myNode->drone.state = 5;
            printf("[%d] Moving to base!\n", i);
        }
        int origin_x = myNode->drone.origin_x;
        int origin_y = myNode->drone.origin_y;
        while(move_base != 0){
            move_base = move_towards(&myNode->drone.d_x, &myNode->drone.d_y, origin_x, origin_y);
            //meter um sleep aqui
            if(myNode->drone.dronePackage != NULL){
                printf("[%d] Got another delivery while returning to base!\n", i);
                move_base = -3;
                break;
            }
        }
        if(move_base == 0){
            printf("[%d] Reached base %d, %d!\n", i, (int)myNode->drone.d_x, (int)myNode->drone.d_y);
            move_base = -3;
        }
        pthread_mutex_unlock(&d_mutex);
        sleep(5);
    }
    pthread_exit(NULL);
    return NULL;

}

DroneList find_drone_node(int drone_id, DroneList droneList){
    DroneList aux = droneList;
    aux = aux->next;
    while(aux != NULL){
        if(aux->drone.drone_id == drone_id){
            return aux;
        }
        aux = aux->next;
    }
    return NULL;
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
    int state=1;
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
    pthread_cond_broadcast(&drone_cond);
    pthread_mutex_unlock(&d_mutex);
    kill_threads();
    exit(0);
}

void create_new_threads(){
    exit_flag = 1;
    kill_threads();
}

void delete_drone_list(DroneList droneList){
    DroneList aux = droneList;
    DroneList next;
    while(aux != NULL){
        next = aux->next;
        free(aux);
        aux = next;
    }
    aux = NULL;
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

    drones_init(droneList, n_drones);
    printf("Creating drone list...\n");
    sleep(5);
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
    cleanup_mq();
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
    int i=999;
    Package order;
    SearchResult result;
    char *prod_string;
    int prod_number;
    int checker;

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
                        asprintf(&prod_string, "Prod_%c", prod_name);
                        //prod number aqui
                        token = strtok(NULL, " ");
                        if(token != NULL && atoi(token) != 0){
                            prod_number = atoi(token);
                            token = strtok(NULL, " ");
                            token = strtok(NULL, ", ");
                            if(token != NULL && atoi(token) != 0){
                                //cord x to deliver
                                int x_deliver = atoi(token);
                                int cord_x = stats_ptr->world_cord_x;
                                int cord_y = stats_ptr->world_cord_y;
                                token = strtok(NULL, " ");
                                if(token != NULL && atoi(token) != 0){
                                    //cord y to deliver
                                    int y_deliver = atoi(token);
                                    if(strtok(NULL, " ") != NULL){
                                        printf("\nInvalid command\n");
                                    }
                                    else{
                                        if(x_deliver > cord_x || y_deliver > cord_y){
                                            printf("Invalid command - Coordinates not withing world coordinates\n");
                                        }
                                        else{
                                            if((checker = check_prod_type(prod_string, prodType))== 0){
                                                printf("Invalid command - Product doesn't exist\n");
                                                result.distance = -2;
                                                result.drone_id = -2;
                                                result.order_x = -2;
                                                result.order_y = -2;
                                                result.w_no = -2;
                                                result.w_x = -2;
                                                result.w_y = -2;
                                            }
                                            else{
                                                i++;
                                                result = goto_closest_warehouse(prod_string, prod_number, x_deliver, y_deliver);
                                                if(result.w_no == -1 || result.drone_id == -1){
                                                    insert_package(i, prod_string, prod_number, y_deliver, x_deliver, packageList);
                                                    list_packages(packageList);
                                                }
                                                else{
                                                    printf("Product is available!\n");
                                        
                                                }
                                            }
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
                            delete_drone_list(droneList);
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
        if(result.distance != -1 && result.distance != -2){
            order.deliver_x = result.order_x;
            order.deliver_y = result.order_y;
            strcpy(order.prod_type, prod_string);
            order.quantity = prod_number;
            order.uid = i;
            order.w_no = result.w_no;
            update_drone_order(droneList, order, result);
            int n_warehouses = stats_ptr->n_warehouses;
            update_warehouse_stock(order, n_warehouses);
            pthread_cond_broadcast(&drone_cond);

        }
    }
}

void update_warehouse_stock(Package order, int n_warehouses){
    Warehouse *aux_ptr = w_ptr;
    for(int i=0; i<n_warehouses; i++){
        for(int j=0; j<3; j++){
            if(strcmp(aux_ptr[i].prodList[j].p_name, order.prod_type) == 0 && aux_ptr[i].w_no == order.w_no){
                aux_ptr[i].prodList[j].quantity -= order.quantity;
                aux_ptr[i].state = 1;
            }
        }
    }
}

void update_drone_order(DroneList droneList, Package order, SearchResult result){
    DroneList aux = droneList;
    aux = aux->next;
    while(aux != NULL){
        if(aux->drone.drone_id == result.drone_id){
            aux->drone.dronePackage = malloc(sizeof(Package));
            aux->drone.dronePackage = &order;
            aux->drone.state = 2;
        }
        aux = aux->next;
    }
}

SearchResult goto_closest_warehouse(char type[50], int quantity, double order_x, double order_y){
    //Usar Variaveis teste por agora *_test
    Warehouse *aux_w = w_ptr;
    DroneList aux_d = droneList->next;
    SearchResult result;
    int warehouse_n = -1;
    double distD_W;
    int drone_id = -1;
    double distMin = 999999;
    int n_warehouses = stats_ptr->n_warehouses;
    for( int i = 0; i < n_warehouses; i ++){
        for(int j = 0; j<3; j++){
            if(strcmp(aux_w[i].prodList[j].p_name, type) == 0){
                if (aux_w[i].prodList[j].quantity > quantity){
                    printf("\nWarehouse found! New warehouse is : %d\n", i);
                    warehouse_n = i;
                }
            }
        }
    }
    if(warehouse_n == -1){
        result.distance = -1;
        result.drone_id = -1;
        result.order_x = -1;
        result.order_y = -1;
        result.w_no = -1;
        result.w_x = -1;
        result.w_y = -1;
        return result;
    }
    else{
        while(aux_d){
            distD_W = distance(aux_d->drone.d_x, aux_d->drone.d_y, aux_w[warehouse_n].w_x, aux_w[warehouse_n].w_y) + distance(aux_d->drone.d_x, aux_d->drone.d_y,order_x, order_y);
            printf("\n\tDist: %f, Drone: %d\n", distD_W, aux_d->drone.drone_id);
            if(distD_W < distMin && (aux_d->drone.state == 1 || aux_d->drone.state == 5)){
                distMin = distD_W;
                drone_id = aux_d->drone.drone_id;
            }
            aux_d = aux_d->next;
        }
        printf("\nWarehouse mais proxima da Warehouse %d ---> Drone %d\n", warehouse_n, drone_id);
    }

    result.distance = distD_W;
    result.drone_id = drone_id;
    result.order_x = order_x;
    result.order_y = order_y;
    result.w_no = warehouse_n;
    result.w_x = aux_w[warehouse_n].w_x;
    result.w_y = aux_w[warehouse_n].w_y;
    return result;
}

int main(){
    signal(SIGINT, signal_handler);
    printf("Simulation manager started\n");
    create_shared_memory();
    read_config();
    open_log_file();
    create_message_queue();
    pid_t pid = getpid();
    printf("SM PID: %d\n", pid);
    srand(time(NULL));
    int supply = stats_ptr->n_warehouses;
    int j=0;
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
        while(1){
            sleep(10);
            supply_warehouses(j);
            j++;
            if(j == supply){
                j=0;
            }
        }
    }
    return 0;
}