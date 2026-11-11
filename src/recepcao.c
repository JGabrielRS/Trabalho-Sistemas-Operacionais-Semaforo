#include "../include/recepcao.h"

// talvez vire double?
static int espera; 

void start_reception(int n, int x){
    espera = x;
    if(n != 0){
        for(int i = 0; i < n; i++)
            create_cliente();
    }
    else infinite_mode(); 
}

void create_cliente(){

}

void infinite_mode(){
    while(1){
        // necessario usar flag para capturar quando s for pressionado e parar o programa
        // necessario verificar se está com menos de 100 clientes
        create_cliente();
    }
}