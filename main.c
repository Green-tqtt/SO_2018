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
int status = 0;
sem_t *access_shared_mem;
sem_t *control_file_write;
FILE *log_file;
PackageList packageList;
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
    stats_ptr->average_time=0;
    stats_ptr->n_drones = 0;
    stats_ptr->Q = 0;
    stats_ptr->S = 0;
    stats_ptr->T = 0.0;
    stats_ptr->avg_d = 0;
    stats_ptr->world_cord_x = 0;
    stats_ptr->world_cord_y = 0;
    stats_ptr->n_warehouses = 0;
	
}
void init_mutex(){
    if (pthread_mutex_init(&d_mutex, NULL) != 0)
    {
        perror("Something went wrong with init mutex\n");
    }
    if (pthread_cond_init(&drone_cond, NULL) != 0)
    {
        perror("Something went wrong with init mutex\n");
    }
    
}

void destroy_mutex(){
    pthread_mutex_destroy(&d_mutex);
    pthread_cond_destroy(&drone_cond);
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
            wh.w_no = i+1;
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

/*
void list_drones(){
    
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
}*/

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

void insert_package(int uid, char prod_type[100], int quantity, int deliver_y, int deliver_x){
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

void list_packages(){
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
            msgrcv(mq_id, &msg_supply, sizeof(msg)-sizeof(long), 100, 0);
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

    //time stuff
   time_t now;
   struct tm *now_tm;
   int hour, minutes, seconds;
   now = time(NULL);
   now_tm = localtime(&now);
   hour = now_tm->tm_hour;
   minutes = now_tm->tm_min;
   seconds = now_tm->tm_sec;
    Warehouse *aux_ptr = w_ptr;
    int random_val = rand() % 3;
    int quantity = stats_ptr->Q;
    aux_ptr[j].prodList[random_val].quantity += quantity;

    msg supply_msg;
    supply_msg.drone_id = 0;
    supply_msg.mtype = 100;
    strcpy(supply_msg.prod_type, aux_ptr[j].prodList[random_val].p_name);
    supply_msg.quantity = quantity;
    aux_ptr[j].state = 2;

    int log_i = j+1;

    fprintf(log_file, "[%d:%d:%d] Warehouse %d got new supply of %s:%d\n", hour, minutes, seconds, log_i, supply_msg.prod_type, supply_msg.quantity);
    
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
    int w_no=0;
    for(int i=0; i<n_warehouses; i++){
        forkVal = fork();
        w_no = i+1;
        if(forkVal == 0){
            printf("[%d] Warehouse w_no: %d is active\n", getpid(), w_no);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Created Warehouse %d with PID: %d\n", hour, minutes, seconds, w_no, getpid());
            sem_post(control_file_write);
            //chama função worker
            warehouse_handler(w_no);
        }
        else if (forkVal < 0){
            perror("Error creating process\n");
            exit(1);
        }
    }
}

void *drone_worker(){
    printf("aaaaaaaaaaaa\n");  
    printf("ENTREI %d\n", 0);
    pthread_exit(NULL);
    return NULL;
}

//------CENTRAL PROCESS-----//
void *drone_handler(void *id){
    int i = *(int *)id;    
    //printf("ENTREI %d\n", i);

    //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;

    int move_warehouse = -3;
    int move_deliver = -3;
    int move_base = -3;
    int quant = 0;
    int d=0;
    printf("[%d]This is my node: %d\n", i, drone_array[i].drone_id);

    while(1){
        pthread_mutex_lock(&d_mutex);
        int w_x = 0;
        int w_y = 0;
        double sleep_val = stats_ptr->T;
        while(exit_flag == 0 && drone_array[i].dronePackage == NULL){
            pthread_cond_wait(&drone_cond, &d_mutex);
        }
        if(exit_flag == 1){
            pthread_mutex_unlock(&d_mutex);
            break;
        }
        pthread_mutex_unlock(&d_mutex);
        int id_w = drone_array[i].dronePackage->w_no;
        w_x = w_ptr[id_w-1].w_x;
        w_y = w_ptr[id_w-1].w_y;
        clock_t t;
        t = clock();
        printf("[%d] I'm ready for a delivery! %s to x:%d, y:%d\n", i, drone_array[i].dronePackage->prod_type, (int)drone_array[i].dronePackage->deliver_x, (int)drone_array[i].dronePackage->deliver_y);
        printf("[%d] Moving to Warehouse %d...\n", i, drone_array[i].dronePackage->w_no);
        while(move_warehouse != 0){
            move_warehouse = move_towards(&drone_array[i].d_x, &drone_array[i].d_y, w_x, w_y);

        }
        if(move_warehouse == 0){
            printf("[%d] Reached warehouse %d, %d!\n", i, (int)drone_array[i].d_x, (int)drone_array[i].d_y);
            drone_array[i].state = 3;
            printf("[%d] Notifying Warehouse...\n", i);
            msg msg_wh;
            msg_wh.mtype = drone_array[i].dronePackage->w_no;
            strcpy(msg_wh.prod_type, drone_array[i].dronePackage->prod_type);
            msg_wh.quantity = drone_array[i].dronePackage->quantity;
            msg_wh.drone_id = drone_array[i].drone_id;
            msg_wh.replyTo = drone_array[i].dronePackage->uid;
            msgsnd(mq_id, &msg_wh, sizeof(msg_wh)-sizeof(long), 0);

            msg msg_rcv;
            int type = drone_array[i].dronePackage->uid;
            sleep_val = drone_array[i].dronePackage->quantity;
            sleep(sleep_val);
            msgrcv(mq_id, &msg_rcv, sizeof(msg)-sizeof(long), type, 0);
            printf("[%d] Supply received from Warehouse %d\n", i, msg_rcv.drone_id);
            quant = drone_array[i].dronePackage->quantity;
            sem_wait(access_shared_mem);
            stats_ptr->n_p_warehouse += quant;
            sem_post(access_shared_mem);
            quant=0;
            drone_array[i].state = 4;
            move_warehouse = -3;

        }
        int order_x = drone_array[i].dronePackage->deliver_x;
        int order_y = drone_array[i].dronePackage->deliver_y;
        while(move_deliver != 0){
            move_deliver = move_towards(&drone_array[i].d_x, &drone_array[i].d_y, order_x, order_y);
            
        }
        if(move_deliver == 0){
            sleep_val = stats_ptr->S;
            sleep(sleep_val);
            t = clock() - t;
            double duration = ((double)t)/CLOCKS_PER_SEC;
            printf("[%d] Reached destination %d, %d!\n", i, (int)drone_array[i].d_x, (int)drone_array[i].d_y);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Drone %d finished delivery %d\n", hour, minutes, seconds, drone_array[i].drone_id, drone_array[i].dronePackage->uid);
            sem_post(control_file_write);
            quant = drone_array[i].dronePackage->quantity;
            move_deliver = -3;
            d++;
            sem_wait(access_shared_mem);
            stats_ptr->n_e_delivered++;
            stats_ptr->n_p_delivered += quant;
            stats_ptr->average_time += duration;
            stats_ptr->avg_d = d;
            sem_post(access_shared_mem);
            drone_array[i].dronePackage = NULL;
            drone_array[i].state = 5;
            quant = 0;
            printf("[%d] Moving to base!\n", i);
        }
        int origin_x = drone_array[i].origin_x;
        int origin_y = drone_array[i].origin_y;
        while(move_base != 0){
            move_base = move_towards(&drone_array[i].d_x, &drone_array[i].d_y, origin_x, origin_y);
            //meter um sleep aqui
            if(drone_array[i].dronePackage != NULL){
                printf("[%d] Got another delivery while returning to base!\n", i);
                move_base = -3;
                break;
            }
        }
        if(move_base == 0){
            printf("[%d] Reached base %d, %d!\n", i, (int)drone_array[i].d_x, (int)drone_array[i].d_y);
            move_base = -3;
        }
    }
    pthread_exit(NULL);
    return NULL;

}

void kill_threads(){
    int n_drones = stats_ptr->n_drones;
    for(int i=0; i<n_drones; i++){
        if(pthread_join(drone_threads[i], NULL)==0){
            printf("[%d] Drone thread has finished\n", drone_array[i].drone_id);
        }
        else{
            perror("Error killing Drone thread\n");
        }
    }
    free(drone_threads);
    free(drone_array);
}

void central_exit(int signum){
    exit_flag=1;
    pthread_cond_broadcast(&drone_cond);
    //pthread_mutex_unlock(&d_mutex);
    kill_threads();
    exit(0);
}

void create_new_threads(){
    exit_flag = 1;
    pthread_cond_broadcast(&drone_cond);
    //pthread_mutex_unlock(&d_mutex);
    kill_threads();
}

void delete_lists(){
    delete_packageList();
    delete_prod_type_list(prodType);
}

void delete_packageList(){
    PackageList aux = packageList;
    PackageList next;
    while(aux != NULL){
        next = aux->next;
        free(aux);
        aux = next;
    }
}

void delete_prod_type_list(ProductTypeList prodType){
    ProductTypeList aux = prodType;
    ProductTypeList next;
    while(aux != NULL){
        next = aux->next;
        free(aux);
        aux = next;
    }
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
    //----
    int n_drones = stats_ptr->n_drones;
    create_threads(n_drones);
    packageList = create_package_list();
    create_named_pipe();
    read_pipe();
    
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

    printf("Creating drone list[%d]...\n", stats_ptr->n_drones);
    
    drone_threads = (pthread_t *)malloc(sizeof(pthread_t)*(stats_ptr->n_drones));
    drone_array = (Drone *)malloc(sizeof(Drone)*(stats_ptr->n_drones));
    //drone_id = malloc(sizeof(int)*n_drones);
    int x,y;
    for(int i = 0; i < stats_ptr->n_drones; i++){
        if (i%4 == 0){
            x = 0;
            y = stats_ptr->world_cord_y;
        }
        else if(i%4 == 1){
            x = stats_ptr->world_cord_x;
            y = stats_ptr->world_cord_y;
        }
        else if(i%4 == 2){
            x = 0;
            y = 0;
        }
        else{
            x = stats_ptr->world_cord_x;
            y = 0;
        }
        drone_array[i].drone_id = i;
        drone_array[i].origin_x = x;
        drone_array[i].d_x = x;
        drone_array[i].origin_y = y;
        drone_array[i].d_y = y;
        drone_array[i].state = 1;
        drone_array[i].dronePackage = NULL;
        printf("%d - %f - %f\n", drone_array[i].drone_id,drone_array[i].d_x,drone_array[i].d_y);
        if((pthread_create(&drone_threads[i], NULL, drone_handler, &drone_array[i].drone_id))!= 0){
            perror("Error creating Drone thread\n");
            exit(1);
        }
        printf("[%d] Drone is active\n", drone_array[i].drone_id);
        sem_wait(control_file_write);
        fprintf(log_file, "[%d:%d:%d] Created Drone %d\n", hour, minutes, seconds, i);
        sem_post(control_file_write);
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
       //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;

    exit_flag=1;
    while(wait(NULL)>0);
    destroy_shared_memory();
    destroy_shared_memory_warehouse();
    unlink_named_pipe();
    cleanup_mq();
    delete_lists();
    sem_wait(control_file_write);
    fprintf(log_file, "[%d:%d:%d] All processes finished!\n", hour, minutes, seconds);
    fprintf(log_file, "[%d:%d:%d] Program finished!\n", hour, minutes, seconds);
    sem_post(control_file_write);
    close_file();
    close_sem();
    destroy_mutex();
    exit(0);
}
void sigusr_handler(int signum){

    printf("----------RESULTS----------\n");
    printf("Número de encomendas atribuídas a drones: %d\n", stats_ptr->n_e_drones);
    printf("Número de produtos carregados nos armazéns: %d\n", stats_ptr->n_p_warehouse);
    printf("Número de encomendas entregues: %d\n", stats_ptr->n_e_delivered);
    printf("Número de produtos entregues: %d\n", stats_ptr->n_p_delivered);
    printf("Tempo médio para conclusão de uma encomenda: %.1f\n", stats_ptr->average_time / stats_ptr->avg_d);
    printf("---------------------------\n");

}

void read_pipe(){

     //time stuff
   time_t now;
   struct tm *now_tm;
   int hour, minutes, seconds;
   now = time(NULL);
   now_tm = localtime(&now);
   hour = now_tm->tm_hour;
   minutes = now_tm->tm_min;
   seconds = now_tm->tm_sec;

    //opens pipe for reading
    if((fd = open(PIPE_NAME, O_RDWR))< 0){
        perror("Error opening pipe for reading: ");
        exit(0);
    }

    char buffer[MAX];
    int bufferlen;
    char *token;
    int i=999;
    Package *order = NULL;
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
                                                sem_wait(control_file_write);
                                                fprintf(log_file, "[%d:%d:%d] Got new order: %d\n", hour, minutes, seconds, i);
                                                sem_post(control_file_write);
                                                result = goto_closest_warehouse(prod_string, prod_number, x_deliver, y_deliver);
                                                if(result.w_no == -1 || result.drone_id == -1){
                                                    insert_package(i, prod_string, prod_number, y_deliver, x_deliver);
                                                    list_packages();
                                                    sem_wait(control_file_write);
                                                    fprintf(log_file, "[%d:%d:%d] Order %d doesn't have enough stock\n", hour, minutes, seconds, i);
                                                    sem_post(control_file_write);
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
                            create_new_threads();
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
        printf("dist %f\n", result.distance);
        printf("d_id %d\n", result.drone_id);
        printf("o_x %f\n", result.order_x);
        printf("o_y %f\n", result.order_y);
        printf("w_no %d\n", result.w_no);

        if(result.distance != -1 && result.distance != -2){
            order = (Package * )malloc(sizeof(Package));
            order->deliver_x = result.order_x;   
            order->deliver_y = result.order_y;
            strcpy(order->prod_type, prod_string);
            order->quantity = prod_number;
            order->uid = i;
            order->w_no = result.w_no;
            drone_array[result.drone_id].dronePackage = order;
            printf("w_no %d drone_id %d \n", drone_array[result.drone_id].dronePackage->w_no, result.drone_id);
            int n_warehouses = stats_ptr->n_warehouses;
            sem_wait(access_shared_mem);
            stats_ptr->n_e_drones++;
            sem_post(access_shared_mem);
            update_warehouse_stock(order, n_warehouses);
            sem_wait(control_file_write);
            fprintf(log_file, "[%d:%d:%d] Drone %d got order %d to deliver\n", hour, minutes, seconds, result.drone_id, order->uid);
            sem_post(control_file_write);
            drone_array[result.drone_id].state = 2;
            pthread_cond_broadcast(&drone_cond);
            
        }
    }
}
Package check_packageList(int n_warehouses){
    PackageList aux = packageList->next;
    PackageList aux_ant = packageList;
    Package order;
    Warehouse *aux_ptr = w_ptr;
    for(int i=0; i<n_warehouses; i++){
        for(int j=0; j<n_warehouses; j++){
            while(aux != NULL){
                if(strcmp(aux_ptr[i].prodList[j].p_name, aux->package.prod_type) == 0 && aux_ptr[i].prodList[j].quantity >= aux->package.quantity){
                    strcpy(order.prod_type, aux->package.prod_type);
                    order.quantity = aux->package.quantity;
                    order.deliver_x = aux->package.deliver_x;
                    order.deliver_y = aux->package.deliver_y;
                    order.uid = aux->package.uid;
                    order.w_no = aux->package.w_no;
                    aux_ant->next = aux->next;
                    free(aux);
                    return order;
                }
            }
            aux_ant = aux;
            aux = aux->next;
        }
    }
    order.deliver_x = -1;
    order.deliver_y = -1;
    strcpy(order.prod_type, "NONE");
    order.quantity = -1;
    order.uid = -1;
    order.w_no = -1;
    return order;
}

int check_empty_list(){
    PackageList aux = packageList->next;
    if(aux->next == NULL){
        return 0;
    }
    return 1;
}

void update_warehouse_stock(Package * order, int n_warehouses){
    Warehouse *aux_ptr = w_ptr;
    for(int i=0; i<n_warehouses; i++){
        for(int j=0; j<3; j++){
            if(strcmp(aux_ptr[i].prodList[j].p_name, order->prod_type) == 0 && aux_ptr[i].w_no == order->w_no){
                aux_ptr[i].prodList[j].quantity -= order->quantity;
                aux_ptr[i].state = 1;
            }
        }
    }
}

SearchResult goto_closest_warehouse(char type[50], int quantity, double order_x, double order_y){
    //Usar Variaveis teste por agora *_test
    Warehouse *aux_w = w_ptr;
    SearchResult result;
    int warehouse_n = -1;
    double distD_W;
    int drone_id = -1;
    double distMin = 999999;
    int n_warehouses = stats_ptr->n_warehouses;
    for( int i = 1; i < n_warehouses+1; i ++){
        for(int j = 0; j<3; j++){
            if(strcmp(aux_w[i-1].prodList[j].p_name, type) == 0){
                if (aux_w[i-1].prodList[j].quantity >= quantity){
                    printf("\nWarehouse found! New warehouse is : %d\n", i);
                    for (int k = 0; k < stats_ptr->n_drones; k++){
                        if (drone_array[k].state == 1 || drone_array[k].state == 5){
                            distD_W = distance(drone_array[k].d_x, drone_array[k].d_y, aux_w[i-1].w_x, aux_w[i-1].w_y) + distance(drone_array[k].d_x, drone_array[k].d_y,order_x, order_y);
                            printf("\n\tDist: %f, Drone: %d\n", distD_W, drone_array[k].drone_id);
                            if(distD_W < distMin){
                                distMin = distD_W;
                                drone_id = drone_array[k].drone_id;
                            }
                        }
                    }
                    warehouse_n = i;
                }
            }
        }
    }
    printf("\nWarehouse mais proxima da Warehouse %d ---> Drone %d\n", warehouse_n, drone_id);
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
        result.distance = distMin;
        result.drone_id = drone_id;
        result.order_x = order_x;
        result.order_y = order_y;
        result.w_no = warehouse_n;
        result.w_x = aux_w[warehouse_n].w_x;
        result.w_y = aux_w[warehouse_n].w_y;
        return result;
    }
}

int main(){
    //time stuff
    time_t now;
    struct tm *now_tm;
    int hour, minutes, seconds;
    int sleep_val = 0;
    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    minutes = now_tm->tm_min;
    seconds = now_tm->tm_sec;
    signal(SIGINT, signal_handler);
    printf("Simulation manager started\n");
    init_sem();
    init_mutex();
    create_shared_memory();
    read_config();
    open_log_file();
    create_message_queue();


    sleep_val = stats_ptr->S;

    signal(SIGUSR1, sigusr_handler);
    pid_t pid = getpid();
    printf("SM PID: %d\n", pid);
    if(getpid() == pid){
        sem_wait(control_file_write);
        fprintf(log_file, "[%d:%d:%d] Program started!\n", hour, minutes, seconds);
        sem_post(control_file_write);
    }
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
        sleep(1);
        warehouse();
        while(1){
            sleep(sleep_val);
            supply_warehouses(j);
            j++;
            if(j == supply){
                j=0;
            }
        }
    }
    return 0;
}