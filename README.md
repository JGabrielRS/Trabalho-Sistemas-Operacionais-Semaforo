# Trabalho de Sistemas Operacionais

Implementação da etapa do **Integrante 1 — Logger e processo Analista** de um trabalho de Sistemas Operacionais desenvolvido em C para Linux.

O objetivo desta parte é registrar os PIDs dos clientes atendidos em um arquivo compartilhado (`lng.txt`) e disponibilizar um processo Analista que lê e imprime esses PIDs em lotes de até 10 registros.

> **Escopo atual:** este repositório contém a implementação do Logger, do Analista e um controlador usado para testar a comunicação entre os dois processos. Ele ainda não representa o sistema completo de Atendimento, Recepção, fila de clientes ou escalonador.

## Tecnologias e requisitos

- Linux;
- GCC;
- GNU Make;
- C11;
- Semáforos nomeados POSIX;
- Chamadas POSIX para criação, sinalização e espera de processos (`fork`, `execve`, `kill` e `waitpid`).

O Makefile utiliza `-pthread` e `-lrt` na etapa de compilação/linkedição.

## Estrutura do projeto

```text
.
├── include/
│   └── logger.h
├── src/
│   ├── analista.c
│   └── logger.c
├── tests/
│   └── test_controlador.c
├── cliente.c
├── Makefile
└── README.md
```

### Componentes

#### Logger — `src/logger.c` e `include/logger.h`

O Logger centraliza o acesso ao arquivo `lng.txt`. O formato utilizado é um PID por linha. Para sincronizar o acesso entre processos, utiliza o semáforo nomeado `/sem_block`.

Interface pública:

```c
int logger_init(void);
int logger_append_pid(pid_t pid);
int logger_process_batch(pid_t pids[], int max_pids);
int logger_has_pending(void);
int logger_destroy(void);
```

Comportamento das funções:

| Função | Descrição | Retorno |
|---|---|---|
| `logger_init()` | Abre/cria o semáforo e garante que `lng.txt` exista. | `0` em sucesso; `-1` em erro. |
| `logger_append_pid(pid)` | Acrescenta um PID ao final do arquivo. | `0` em sucesso; `-1` em erro. |
| `logger_process_batch(pids, max_pids)` | Lê até `max_pids` PIDs, coloca-os no vetor e preserva no arquivo os registros restantes. | Quantidade de PIDs obtidos; `-1` em erro. |
| `logger_has_pending()` | Verifica se há ao menos um PID registrado. | `1` se houver; `0` se estiver vazio; `-1` em erro. |
| `logger_destroy()` | Fecha a referência local ao semáforo e, no processo que o criou, remove seu nome. | `0` em sucesso; `-1` em erro. |

Durante o processamento de um lote, o Logger escreve os registros restantes em `lng.txt.tmp` e, ao concluir, substitui `lng.txt` usando `rename()`.

#### Processo Analista — `src/analista.c`

O Analista inicializa o Logger e aguarda sua ativação por sinal. O protocolo implementado utiliza:

- `SIGSTOP` para suspender o Analista enquanto aguarda;
- `SIGCONT` para retomá-lo e solicitar o processamento de um lote;
- `logger_process_batch()` para obter até 10 PIDs;
- `printf()` para imprimir os PIDs recebidos.

Quando é ativado e não encontra registros pendentes, o Analista encerra normalmente. O Atendimento (ou o controlador de teste) deve enviar essa ativação final somente depois de concluir a geração de registros.

#### Controlador de teste — `tests/test_controlador.c`

O controlador simula parte do comportamento do processo Atendimento. Ele:

1. Inicializa o Logger;
2. cria o processo Analista com `fork()` e executa `./analista` com `execve()`;
3. aguarda a suspensão inicial do Analista com `waitpid()` e `WUNTRACED`;
4. insere 25 PIDs fictícios (`1000` a `1024`) no arquivo LNG;
5. ativa o Analista com `SIGCONT` e aguarda cada lote;
6. envia a ativação final quando o LNG fica vazio;
7. aguarda o encerramento do Analista e libera os recursos.

O arquivo `cliente.c` também está presente no repositório, mas não é compilado nem utilizado pelo teste atual do Logger/Analista.

## Compilar e executar

Execute os comandos a partir da raiz do repositório, pois os caminhos `./analista`, `lng.txt` e `lng.txt.tmp` são relativos ao diretório de trabalho atual.

### Compilar

```bash
make
```

Esse comando gera os executáveis `analista` e `controlador`.

### Executar o teste

```bash
make test
```

O alvo `test` remove os arquivos `lng.txt` e `lng.txt.tmp` de uma execução anterior e executa o controlador. O teste atual deve produzir três lotes: 10 PIDs, mais 10 PIDs e, por fim, 5 PIDs. Depois, uma ativação adicional permite que o Analista detecte o LNG vazio e finalize.

### Limpar os artefatos gerados

```bash
make clean
```

Esse comando remove os executáveis `analista` e `controlador`, além de `lng.txt` e `lng.txt.tmp`.

## Resultado do teste atual

Com a implementação presente neste repositório, `make clean`, `make` e `make test` foram executados com sucesso em ambiente Linux. O cenário automatizado verifica o fluxo com 25 PIDs fictícios e o encerramento normal do Analista.

Isso não substitui testes adicionais com arquivos vazios e lotes de 1, 9, 10 e 11 PIDs, testes de concorrência entre processos ou a integração com o Atendimento completo.

## Observações para integração

- O Atendimento e o Analista precisam executar com o mesmo diretório de trabalho, para acessar o mesmo `lng.txt` e `lng.txt.tmp`.
- O semáforo `/sem_block` deve permanecer disponível enquanto houver processos utilizando o Logger. O processo responsável por criá-lo deve removê-lo somente depois que o Analista tiver terminado.
- O Atendimento deve aguardar o Analista voltar ao estado suspenso antes de enviar uma nova ativação. Sinais padrão não devem ser tratados como uma fila de notificações.
- A implementação atual de `logger_process_batch()` remove os PIDs do LNG antes de devolvê-los ao Analista para impressão. Portanto, ela cobre o fluxo normal do teste, mas ainda não oferece confirmação posterior da impressão caso o Analista falhe entre a leitura e a exibição do lote.
- A integração com o escalonador de threads do Atendimento precisa considerar que `waitpid()` bloqueante pode bloquear o processo inteiro em modelos de threads implementadas em espaço de usuário.

## Estado do desenvolvimento

- [x] Interface inicial do Logger;
- [x] Registro de PIDs no LNG;
- [x] Leitura e remoção de lotes;
- [x] Sincronização do arquivo com semáforo nomeado;
- [x] Processo Analista com ativação por sinais;
- [x] Controlador para teste isolado com 25 PIDs;
- [ ] Testes automatizados para diferentes tamanhos de lote;
- [ ] Protocolo de confirmação da impressão dos lotes;
- [ ] Integração com o processo Atendimento e seu escalonador.