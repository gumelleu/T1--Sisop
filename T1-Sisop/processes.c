
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define TOTAL    1000000000LL
#define SEM_NAME "/t1_sisop_counter_sem"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N> <sync>\n", argv[0]);
        fprintf(stderr, "  N    = numero de processos (ex: 2, 4, 8)\n");
        fprintf(stderr, "  sync = 0 (P1: sem semaforo) | 1 (P2: com semaforo)\n");
        return EXIT_FAILURE;
    }

    int n        = atoi(argv[1]);
    int use_sem  = atoi(argv[2]);

    if (n <= 0) {
        fprintf(stderr, "Erro: N deve ser positivo.\n");
        return EXIT_FAILURE;
    }

    printf("=== Experimento %s | N=%d processos | Total=%lld ===\n",
           use_sem ? "P2 (semaforo)" : "P1 (sem sync)", n, TOTAL);
    /* Limpa o buffer antes do fork para evitar impressao duplicada nos filhos */
    fflush(stdout);

    /* --- Memoria Compartilhada (shmget / shmat) --- */
    int shmid = shmget(IPC_PRIVATE, sizeof(long long), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return EXIT_FAILURE;
    }

    /* shmat retorna void* — guardamos ponteiro bruto para shmdt/free */
    void *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    /*
     * volatile: garante que cada processo leia/escreva diretamente na
     * memoria compartilhada, sem cache em registrador. Essencial para
     * que P1 demonstre a race condition real entre processos.
     */
    volatile long long *counter = (volatile long long *)shm_ptr;
    *counter = 0;

    /* --- Semaforo POSIX nomeado (P2 apenas) --- */
    sem_t *sem = SEM_FAILED;
    if (use_sem) {
        /* Remove semaforo anterior caso tenha ficado pendente */
        sem_unlink(SEM_NAME);
        sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
        if (sem == SEM_FAILED) {
            perror("sem_open");
            shmdt(shm_ptr);
            shmctl(shmid, IPC_RMID, NULL);
            return EXIT_FAILURE;
        }
    }

    long long per_proc = TOTAL / n;

    /* --- Cria N processos filhos via fork --- */
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            /* Limpa recursos e encerra */
            shmdt(shm_ptr);
            shmctl(shmid, IPC_RMID, NULL);
            if (use_sem) { sem_close(sem); sem_unlink(SEM_NAME); }
            return EXIT_FAILURE;

        } else if (pid == 0) {
            /* PROCESSO FILHO */
            long long iters = (i == n - 1) ? TOTAL - per_proc * (n - 1)
                                           : per_proc;

            if (use_sem) {
                /* P2: protege cada incremento com semaforo */
                for (long long j = 0; j < iters; j++) {
                    sem_wait(sem);
                    (*counter)++;
                    sem_post(sem);
                }
                sem_close(sem);
            } else {
                /* P1: sem protecao - race condition intencional */
                for (long long j = 0; j < iters; j++) {
                    (*counter)++;
                }
            }

            shmdt(shm_ptr);
            exit(EXIT_SUCCESS);
        }
        /* Processo pai continua o loop para criar os demais filhos */
    }

    /* PROCESSO PAI: aguarda todos os filhos */
    for (int i = 0; i < n; i++) {
        int status;
        wait(&status);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Aviso: processo filho terminou com erro.\n");
        }
    }

    long long final_val = *counter;
    printf("Contador final : %lld\n", final_val);
    printf("Esperado       : %lld\n", TOTAL);
    printf("Diferenca      : %lld\n", TOTAL - final_val);

    /* Limpeza de recursos */
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);

    if (use_sem) {
        sem_close(sem);
        sem_unlink(SEM_NAME);
    }

    return EXIT_SUCCESS;
}
