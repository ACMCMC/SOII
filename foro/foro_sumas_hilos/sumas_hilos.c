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

void sum(int hilo)
{
    int j;
    for (j = hilo; j <= M; j += T)
    {
        suma = suma + j;
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
    double valor_correcto = (double) atoi(argv[3]);
    pthread_t *ids = (pthread_t *)malloc(sizeof(pthread_t) * T);
    for (i = 0; i < T; i++) {
    pthread_create(ids+i, NULL, sum, i);
    }
    for (i = 0; i < T; i++) {
        pthread_join(ids[i], NULL);
    }
    if (fabs(valor_correcto - suma) > 0.01) { // Si el valor no coincide, salimos con la diferencia que ha habido
        printf("Valor incorrecto. Debía ser %lf, es %lf\n", valor_correcto, suma);
        return (int) fabs(valor_correcto-suma);
    }
    free(ids);
    return (0);
}
