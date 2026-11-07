#define _POSIX_C_SOURCE 200809L

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

extern char **environ;

static int esperar_analista(pid_t pid)
{
    int status;
    pid_t resultado;

    do {
        resultado = waitpid(pid, &status, WUNTRACED);
    } while (resultado == -1 && errno == EINTR);

    if (resultado == -1) {
        perror("[Controlador] Erro em waitpid");
        return -1;
    }

    if (WIFSTOPPED(status)) {

        if (WSTOPSIG(status) == SIGSTOP) {
            printf("[Controlador] Analista está aguardando.\n");
            return 0;
        }

        fprintf(stderr, "[Controlador] Analista parou com sinal inesperado.\n");
        return -1;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr,
                "[Controlador] Analista encerrou inesperadamente. Código: %d\n",
                WEXITSTATUS(status));
        return 1;
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr,
                "[Controlador] Analista foi encerrado por sinal: %d\n",
                WTERMSIG(status));
        return 1;
    }

    return -1;
}

int main(void)
{
    pid_t pid_analista;
    int pendentes;
    int resultado;
    int status;
    int analista_ativo = 0;
    int sucesso = 0;

    printf("========== TESTE DO ANALISTA ==========\n\n");

    if (logger_init() == -1) {
        perror("[Controlador] logger_init");
        return EXIT_FAILURE;
    }

    pid_analista = fork();

    if (pid_analista == -1) {
        perror("[Controlador] fork");
        logger_destroy();
        return EXIT_FAILURE;
    }

    if (pid_analista == 0) {

        char *args[] = {"./analista", NULL};

        execve("./analista", args, environ);

        perror("[Controlador] execve");
        _exit(127);
    }

    analista_ativo = 1;

    printf("[Controlador] Analista criado. PID: %ld\n",
           (long)pid_analista);

    resultado = esperar_analista(pid_analista);

    if (resultado != 0) {
        if (resultado == 1)
            analista_ativo = 0;

        goto finalizar;
    }

    printf("\n[Controlador] Inserindo 25 PIDs no LNG...\n");

    for (int i = 1000; i < 1025; i++) {

        if (logger_append_pid(i) == -1) {
            perror("[Controlador] logger_append_pid");
            goto finalizar;
        }
    }

    printf("[Controlador] PIDs inseridos com sucesso!\n");

    while (1) {

        pendentes = logger_has_pending();

        if (pendentes == -1) {
            perror("[Controlador] logger_has_pending");
            goto finalizar;
        }

        if (pendentes == 0)
            break;

        printf("\n[Controlador] Enviando SIGCONT...\n");

        if (kill(pid_analista, SIGCONT) == -1) {
            perror("[Controlador] kill SIGCONT");
            goto finalizar;
        }

        resultado = esperar_analista(pid_analista);

        if (resultado != 0) {
            if (resultado == 1)
                analista_ativo = 0;

            goto finalizar;
        }
    }

    printf("\n[Controlador] LNG vazio!\n");
    printf("[Controlador] Enviando ativação final...\n");

    if (kill(pid_analista, SIGCONT) == -1) {
        perror("[Controlador] kill final");
        goto finalizar;
    }

    pid_t aguardado;

    do {
        aguardado = waitpid(pid_analista, &status, 0);
    } while (aguardado == -1 && errno == EINTR);

    if (aguardado == -1) {
        perror("[Controlador] waitpid final");
        goto finalizar;
    }

    analista_ativo = 0;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "[Controlador] Analista terminou com erro.\n");
        goto finalizar;
    }

    printf("\n[Controlador] Analista finalizado corretamente!\n");

    pendentes = logger_has_pending();

    if (pendentes != 0) {
        fprintf(stderr, "[Controlador] Ainda existem PIDs pendentes!\n");
        goto finalizar;
    }

    sucesso = 1;

finalizar:

    if (analista_ativo) {
        kill(pid_analista, SIGKILL);

        while (waitpid(pid_analista, NULL, 0) == -1 &&
               errno == EINTR) {
        }
    }

    if (logger_destroy() == -1) {
        perror("[Controlador] logger_destroy");
        sucesso = 0;
    }

    printf("\n=======================================\n");

    if (sucesso) {
        printf("TESTE CONCLUÍDO COM SUCESSO!\n");
    } else {
        printf("TESTE FINALIZADO COM ERRO!\n");
    }

    printf("=======================================\n");

    return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
}