#include "header.h"
int shmid;
int fd; 
Stats *stats_ptr;
int power = 1;
pthread_t *drone_threads;
int *drone_id;
int retStat;
pid_t wpid;
int status = 0;


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

void destroy_thread_pool(){
	int drone_id[4];
    pthread_t drone_thread[stats_ptr->n_drones];
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

    if((fd = open(PIPE_NAME, O_RDONLY))< 0){
        perror("Error opening pipe for reading: ");
        exit(0);
    }

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

//------WAREHOUSE PROCESS-----//
void warehouse_handler(int i){
    stats_ptr->wArray[i]->w_no = getpid();
    printf("[%d] Hello! I'm a warehouse!\n", getpid());
    /*printf("W_NAME: %s\n", stats_ptr->wArray[i]->w_name);
    printf("W_XY: %f\n", stats_ptr->wArray[i]->w_x);
    printf("W_NAME: %f\n", stats_ptr->wArray[i]->w_y);
    printf("W_NO: %d\n", stats_ptr->wArray[i]->w_no);
    printf("\t\tPRODUCT LIST\n");
    list_product(stats_ptr->wArray[i]->prodList);
    printf("\n");*/
    sleep(3);
    printf("[%d] Goodbye!\n", getpid());
    exit(0);
}

void warehouse(){
    pid_t pid = getpid();
    int forkVal;
    for(int i=0; i<stats_ptr->n_warehouses; i++){
        forkVal = fork();
        if(forkVal == 0){
            pid = getpid();
            printf("[%d] Warehouse %d is active\n", getpid(), i+1);
            stats_ptr->wArray[i]->w_no = getpid();
            //chama função worker
            warehouse_handler(i);
        }
        else if (forkVal < 0){
        perror("Error creating process\n");
        exit(0);
        }
    }
    //while((wpid = wait(&status) > 0));
}

//------CENTRAL PROCESS-----//
void *drone_handler(void *id){
    int i = *(int*)id;
    printf("[%d] Drone doing stuff\n", drone_id[i]);
    sleep(5);
    printf("[%d] I'm done\n", drone_id[i]);
    pthread_exit(NULL);
}

void kill_threads(){
    for(int i=1; i<=stats_ptr->n_drones; i++){
        if(pthread_join(drone_threads[i], NULL)==0){
            printf("[%d] Drone thread has finished\n", drone_id[i]);
        }
        else{
            perror("Error killing Drone thread\n");
        }
    }
}

void central(){
	int i;
    drone_threads = malloc(sizeof(pthread_t)*stats_ptr->n_drones);
    drone_id = malloc(sizeof(int)*stats_ptr->n_drones);
    for(i = 1; i <= stats_ptr->n_drones; i++){
        drone_id[i] = i;
        if(pthread_create(&drone_threads[i], NULL, drone_handler, &drone_id[i])==0){
            printf("[%d] Drone is active\n", drone_id[i]);
        }
        else{
            perror("Error creating Drone thread\n");
        }
    }
}


int main(){
	/*signal(SIGINT, signal_handler);
    printf("oi");
    read_config();
    create_thread_pool();
    create_shared_memory();
    create_process();
    while(power){
    }*/
    printf("Simulation manager started\n");
    create_shared_memory();
    //create_named_pipe();
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
    }
}