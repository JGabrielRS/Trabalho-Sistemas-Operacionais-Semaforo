#define _POSIX_C_SOURCE 200809L

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MAX_PIDS 10

int main(void)
{
    pid_t pids[MAX_PIDS];
    int quantidade;

    if (logger_init() == -1) {
        perror("[Analista] Erro ao inicializar Logger");
        return EXIT_FAILURE;
    }

    printf("[Analista] Iniciado. PID: %ld\n", (long)getpid());
    fflush(stdout);

    while (1) {

        if (raise(SIGSTOP) != 0) {
            perror("[Analista] Erro ao suspender processo");
            logger_destroy();
            return EXIT_FAILURE;
        }

        printf("\n[Analista] Ativado!\n");

        quantidade = logger_process_batch(pids, MAX_PIDS);

        if (quantidade == -1) {
            perror("[Analista] Erro ao processar lote");
            logger_destroy();
            return EXIT_FAILURE;
        }

        if (quantidade == 0) {
            printf("[Analista] Nenhum PID pendente.\n");
            printf("[Analista] Encerrando processo.\n");
            fflush(stdout);
            break;
        }

        printf("[Analista] Processando %d PID(s):\n", quantidade);

        for (int i = 0; i < quantidade; i++) {
            printf("  PID: %ld\n", (long)pids[i]);
        }

        printf("[Analista] Lote finalizado.\n");

        fflush(stdout);

    }

    if (logger_destroy() == -1) {
        perror("[Analista] Erro ao finalizar Logger");
        return EXIT_FAILURE;
    }

    printf("[Analista] Finalizado com sucesso.\n");

    return EXIT_SUCCESS;
}