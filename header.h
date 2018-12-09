
/*H**********************************************************************
* FILENAME :   	drone_movement.h
*
* DESCRIPTION :
*       		Support code for the Operating Systems (SO)
*				Practical Assignment 2018.
*
* PUBLIC FUNCTIONS :
* 		double 	distance ( x1,  y1,  x2,  y2 )
*		int 	move_towards( *drone_x,  *drone_y, target_x, target_y )
*
*
* AUTHOR :  	Nuno Antunes
* DATE   :  	2018/09/14
*
*H*/

#ifndef SO2018_DRONE_MOVEMENT_H
#define SO2018_DRONE_MOVEMENT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/msg.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/timeb.h>
#include <math.h>
#define PIPE_NAME "input_pipe"
#define DISTANCE 1
#define MAX 255


//criacao de struct product type que vai conter os produtos especificados
typedef struct productType{
    char p_name[50];
}Product_type;

//pointer para product_type
Product_type* type_ptr;

//vai servir para comparar se os produtos no inicio do warehouse são válidos
typedef struct product_type_node *ProductTypeList;
typedef struct product_type_node{
    Product_type product_type;
    ProductTypeList next;
}type_node;

//servir para utilizar na warehouse porque da jeito ter os produtos e as quantidades
typedef struct product{
    char p_name[50];
    int quantity;
    //em que warehouse é que esse produto está
}Product;
//pointer para a struct product
Product *pw_ptr;

//struct de return apos uma pesquisa de uma warehouse com a encomenda e escolhido um drone
typedef struct searchResult{
    int drone_id;
    double w_x, w_y, order_x, order_y;
    int w_no;
    double distance;

}SearchResult;

//cada warehouse vai ter a sua lista ligada de produtos com o seu nome e a sua quantidade
typedef struct warehouse{
    char w_name[50];
    double w_x;
    double w_y;
    int w_no;
    Product prodList[3];
    int state; //0- free    1-handling delivery  2-resupply
}Warehouse;
//pointer para a struct warehouse
Warehouse *w_ptr;

typedef struct package{
    int uid;
    char prod_type[100];
    int quantity;
    double deliver_x;
    double deliver_y;
    int w_no;
}Package;
Package *pack_ptr;

typedef struct p_node *PackageList;
typedef struct p_node{
    Package package;
    PackageList next;
}package_node;


typedef struct drone{
    Package *dronePackage;
    int drone_id;
    int state;
    double d_x, d_y, origin_x, origin_y;
}Drone;
Drone *drone_ptr;

//lista ligada de drones
typedef struct d_node *DroneList;
typedef struct d_node{
    Drone drone;
    DroneList next;
}drone_node;

typedef struct stats{
    int n_e_drones; //Número total de encomendas atribuidas a drones
    int n_p_warehouse; // Número total de produtos carregados de armazéns
    int n_e_delivered; //Número total de encomendas entregues
    int n_p_delivered; //Número total de produtos entregues
    double average_time; //Tempo médio de conclusão de uma encomenda
    int avg_d;
    int world_cord_x; //coordenadas do mundo
    int world_cord_y; //cord do mundo
    int n_drones; //numero de drones
    int S, Q;//unidades de tempo
    double T;
    int n_warehouses; //numero de warehouses
}Stats;
Stats *stats_ptr;

typedef struct message{
    long mtype;
    int drone_id;
    char prod_type[50];
    int quantity;
    int replyTo;
}msg;


/*
 * Computes the distance between two points
 */
double distance(double x1, double y1, double x2, double y2);

/*
 * Advances the drone 1 step towards the target point.
 *
 * You should provide the address for the coordinates of your drone,
 * in order for them to be updated, according to the following cases
 *
 *
 * returns  1 - in case it moved correctly towards the target
 *				drone_x, drone_y will be updated
 *
 * 
 * returns  0 - in case it moved and reached the target
 *				drone_x, drone_y will be updated
 *  
 * returns -1 - in case it was already in the target
 *				drone_x, drone_y are NOT updated
 * returns -2 - in case there is an error
 *				drone_x, drone_y are NOT updated
 */
int move_towards(double *drone_x, double *drone_y, double target_x, double target_y);

void create_warehouse_sm(int n_warehouses);
void init_sem();
void close_sem();
void create_message_queue();
void cleanup_mq();
void destroy_shared_memory();
void destroy_shared_memory_warehouse();
void read_config();
void open_log_file();
void close_file();
void create_named_pipe();
ProductTypeList create_product_type_list(void);
void insert_product_type(char p_name[50], ProductTypeList productType);
void list_product_types(ProductTypeList productType);
int check_prod_type(char p_name[50], ProductTypeList productType);
DroneList create_drone_list(void);
void insert_drone(int drone_id, int state, double d_x, double d_y, DroneList droneList);
void list_drones(DroneList droneList);
void processes_exit();
PackageList create_package_list(void);
void insert_package(int uid, char prod_type[100], int quantity, int deliver_y, int deliver_x, PackageList packageList);
void list_packages(PackageList packageList);
void warehouse_handler(int i);
void warehouse();
void *drone_handler(void *id);
DroneList find_drone_node(int drone_id, DroneList droneList);
void drones_init(DroneList droneList, int n_drones);
void central_exit(int signum);
void create_new_threads();
void delete_drone_list(DroneList droneList);
void central();
void create_threads(int n_drones);
void unlink_named_pipe();
void signal_handler(int signum);
void read_pipe(PackageList packageList, DroneList droneList);
void update_warehouse_stock(Package order, int n_warehouses);
void update_drone_order(DroneList droneList, Package order, SearchResult result);
SearchResult goto_closest_warehouse(char type[50], int quantity, double order_x, double order_y);
void supply_warehouses(int j);
int check_empty_list(PackageList packageList);
void delete_lists();
void delete_drone_list(DroneList droneList);
void delete_packageList(PackageList packageList);
void delete_prod_type_list(ProductTypeList prodType);
Package check_packageList(PackageList packageList, int n_warehouses);
void sigusr_handler(int signum);

#endif
//kill -USR1 piddoprocesso
