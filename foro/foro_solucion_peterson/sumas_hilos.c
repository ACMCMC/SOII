// Aldán Creo Mariño, SOII 2020/21

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

double suma = 0;

int T, M;

#define FALSE 0
#define TRUE 1
#define N 2
int turno;
int interesado[N];
void entrar_region(int proceso)
{
    int otro;
    otro = 1 - proceso;
    interesado[proceso] = TRUE;
    turno = proceso;
    while (turno == proceso && interesado[otro] == TRUE) {};
}
void salir_region(int proceso)
{
    interesado[proceso] = FALSE;
}

void sum(int hilo)
{
    int j;
    for (j = hilo; j <= M; j += T)
    {
        entrar_region(hilo);
        suma = suma + j;
        salir_region(hilo);
    }
}

int main(int argc, char **argv)
{
    int i;
    if (argc != 4)
    {
        fprintf(stderr, "Debe especificarse: %s [M] [T] [valor correcto de la suma]\n", argv[0]);
    }
    M = atoi(argv[1]);
    T = atoi(argv[2]);
    double valor_correcto = (double)atoi(argv[3]);
    pthread_t *ids = (pthread_t *)malloc(sizeof(pthread_t) * T);
    for (i = 0; i < T; i++)
    {
        pthread_create(ids + i, NULL, sum, i);
    }
    for (i = 0; i < T; i++)
    {
        pthread_join(ids[i], NULL);
    }
    printf("%d,%d,%d\n", M, T, (int) fabs(valor_correcto - suma));
    free(ids);
    return (EXIT_SUCCESS);
}
