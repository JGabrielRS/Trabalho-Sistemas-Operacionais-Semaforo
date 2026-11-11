#ifndef FILA_H
#define FILA_H

typedef struct cliente{
    pid_t PID;
    double h_chegada;
    int prioridade;
    double t_atendimento;
} cliente;

void add_cliente(cliente c);
cliente next_cliente();
// criar semaforo para limitar a 100 clientes?

#endif