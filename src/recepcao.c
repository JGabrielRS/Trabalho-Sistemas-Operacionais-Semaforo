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

char *args[] = {"./cliente", NULL};
void create_cliente(){
    pid_t id = fork();

    if(id < 0){
        perror("Erro no fork");
        exit(1);
    }
    
    if(id == 0){
        execv("../cliente.c", args);
        perror("Erro no execv");
        exit(1);
    } else {
        int prior = rand() % 2;
        //add_cliente({id, h_chegada, prior, t_atend});
    }
}

void infinite_mode(){
    while(1){
        // necessario usar flag para capturar quando s for pressionado e parar o programa
        // necessario verificar se está com menos de 100 clientes
        create_cliente();
    }
}