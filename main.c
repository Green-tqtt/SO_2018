#include "header.h"
int world_cord_x;
int world_cord_y;
Warehouse *sample;
ProductTypeList productType;

int main(){
    printf("oi");
    read_config();
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
