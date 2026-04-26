
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TOTAL 1000000000LL


static volatile long long counter = 0;

/* Mutex para o experimento T2 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Controle de modo: 0 = sem sync, 1 = com mutex */
static int use_mutex = 0;

/* Argumento passado para cada thread */
typedef struct {
    long long iterations; /* quantidade de incrementos a fazer */
} ThreadArg;

/* Funcao executada por cada thread */
static void *worker(void *arg)
{
    ThreadArg *targ = (ThreadArg *)arg;
    long long iters = targ->iterations;

    if (use_mutex) {
        /* T2: protege cada incremento com mutex */
        for (long long i = 0; i < iters; i++) {
            pthread_mutex_lock(&mutex);
            counter++;
            pthread_mutex_unlock(&mutex);
        }
    } else {
        /* T1: sem protecao - race condition intencional */
        for (long long i = 0; i < iters; i++) {
            counter++;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N> <sync>\n", argv[0]);
        fprintf(stderr, "  N    = numero de threads (ex: 2, 4, 8)\n");
        fprintf(stderr, "  sync = 0 (T1: sem mutex) | 1 (T2: com mutex)\n");
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    use_mutex = atoi(argv[2]);

    if (n <= 0) {
        fprintf(stderr, "Erro: N deve ser positivo.\n");
        return EXIT_FAILURE;
    }

    printf("=== Experimento %s | N=%d threads | Total=%lld ===\n",
           use_mutex ? "T2 (mutex)" : "T1 (sem sync)", n, TOTAL);

    pthread_t   threads[n];
    ThreadArg   args[n];
    long long   per_thread = TOTAL / n;

    /* Cria as N threads */
    for (int i = 0; i < n; i++) {
        /* Ultima thread pega o restante da divisao */
        args[i].iterations = (i == n - 1) ? TOTAL - per_thread * (n - 1)
                                           : per_thread;

        if (pthread_create(&threads[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    /* Aguarda todas as threads terminarem */
    for (int i = 0; i < n; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
    }

    pthread_mutex_destroy(&mutex);

    printf("Contador final : %lld\n", counter);
    printf("Esperado       : %lld\n", TOTAL);
    printf("Diferenca      : %lld\n", TOTAL - counter);

    return EXIT_SUCCESS;
}
