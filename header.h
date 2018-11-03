
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

#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>


#define DISTANCE 1


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
    char w_name[50];
}Product;
//pointer para a struct product
Product *pw_ptr;

//lista ligada de products de cada warehouse
typedef struct Product_node *ProductList;
typedef struct Product_node{
    Product product;
    ProductList next;
}product_node;

//cada warehouse vai ter a sua lista ligada de produtos com o seu nome e a sua quantidade
typedef struct warehouse{
    char w_name[50];
    float w_x;
    float w_y;
}Warehouse;
//pointer para a struct warehouse
Warehouse *w_ptr;

typedef struct package{
    int uid;
    char prod_type[50];
    int quantity;
    float deliver_x;
    float deliver_y;
}Package;
Package *pack_ptr;

typedef struct drone{
    int drone_id;
    int state;
    float d_x, d_y, dest_x, dest_y;
}Drone;
Drone *drone_ptr;

typedef struct stats{
    int n_e_drones; //Número total de encomendas atribuidas a drones
    int n_p_warehouse; // Número total de produtos carregados de armazéns
    int n_e_delivered; //Número total de encomendas entregues
    int n_p_delivered; //Número total de produtos entregues
    float average_time; //Tempo médio de conclusão de uma encomenda
}Stats;
Stats *stats_ptr;


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

void signal_handler(int signum);
void create_thread_pool();
void destroy_thread_pool();
void *drone_action();
void create_shared_memory();
void destroy_shared_memory();
void read_config();
ProductTypeList create_product_type_list(void);
void insert_product_type(char p_name[50], ProductTypeList productType);
void list_product_types(ProductTypeList productType);


#endif
