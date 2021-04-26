#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#define P 5              // productores
#define C 4              // consumidores
#define N 5              // tamaño del buffer
#define ITEMS_BY_P 10    // items por cada productor
#define SLEEP_MAX_TIME 4 // máximo tiempo para producir / consumir

int buffer[N]; // El buffer compartido de memoria
int cuenta;    // El número de elementos de la cola
int primero;   // El lugar donde está el primero de la cola

pthread_mutex_t the_mutex;
pthread_cond_t condp, condc;

// Imprime por pantalla una representacion grafica del buffer. La parte ocupada de la lista se imprime en rojo, la parte libre en azul.
void imprimir_buffer()
{
    int i;
    printf("\t-");
    for (i = 0; i < N; i++)
    {
        printf("------");
    }
    printf("\n\t|");
    for (i = 0; i < N; i++)
    {
        printf(" %-3d |", buffer[i]);
    }
    printf("\n\t-");
    for (i = 0; i < N; i++)
    {
        printf("------");
    }
    printf("\n\n");
}

// Inserta un item en el buffer
void insertar_item_buffer(int item)
{
    buffer[(primero + cuenta) % N] = item;
    cuenta++;
}

// Saca un item del buffer y escribe un 0 en su lugar
int sacar_item_buffer()
{
    int elem = buffer[primero];
    buffer[primero] = 0;
    primero = (primero + 1) % N;
    cuenta--;
    return elem;
}

// Produce un item, para introducirlo en el buffer
int producir()
{
    sleep(((int)rand()) % SLEEP_MAX_TIME); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
    return ((int)rand() % 999) + 1;        // Numeros entre 1 y 999
}

// Consume un item del buffer
void consumir(int numero)
{
    sleep(((int)rand()) % SLEEP_MAX_TIME); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
}

// Implementa la funcion de las diapositivas
void *productor(void *p_num_elementos)
{
    int num_elementos_restantes; // El numero de elementos que faltan por producir
    int item;                    // El item que estamos produciendo actualmente
    int num_elementos = *((int *)p_num_elementos);

    for (num_elementos_restantes = 0; num_elementos_restantes < num_elementos; num_elementos_restantes++)
    {
        item = producir();
        pthread_mutex_lock(&the_mutex);
        printf("(C) Adquiero el mutex\n");
        while (cuenta == N)
        {
            pthread_cond_wait(&condp, &the_mutex);
        }
        insertar_item_buffer(item);
        printf("(P) Inserto: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        pthread_cond_signal(&condc);
        pthread_mutex_unlock(&the_mutex);
    }
    printf("(P) He acabado!\n");
    return NULL;
}

// Implementa la funcion de las diapositivas
void *consumidor(void *p_num_elementos)
{
    int num_elementos_restantes; // El numero de elementos que faltan por consumir
    int item;                    // El item que estamos consumiendo actualmente
    int num_elementos = *((int *)p_num_elementos);

    for (num_elementos_restantes = 0; num_elementos_restantes < num_elementos; num_elementos_restantes++)
    {
        pthread_mutex_lock(&the_mutex);
        printf("(C) Adquiero el mutex\n");
        while (cuenta == 0)
        {
            pthread_cond_wait(&condc, &the_mutex);
        }
        item = sacar_item_buffer();
        printf("(C) Saco: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&the_mutex);
        consumir(item);
    }
    printf("(C) He acabado!\n");
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads_productores[P];
    pthread_t threads_consumidores[C];
    int num_elementos_productores[P];
    int num_elementos_consumidores[C];
    int i;

    srand(clock()); // Semilla del generador aleatorio de numeros

    cuenta = 0; // Inicializamos la variable cuenta a 0. Deberia venir ya inicializada, pero creo que es buena practica asegurarnos.
    primero = 0;

    pthread_mutex_init(&the_mutex, 0);
    pthread_cond_init(&condp, 0);
    pthread_cond_init(&condc, 0);

    for (i = 0; i < P; i++)
    {
        num_elementos_productores[i] = ITEMS_BY_P;
        if (pthread_create(&threads_productores[i], NULL, &productor, (void *)&num_elementos_productores[i]))
        { // Hubo error, aun asi no abortamos
            perror("Error en pthread_create()");
        }
        printf("Generado el productor %d\n", i);
    }
    for (i = 0; i < C; i++)
    {
        num_elementos_consumidores[i] = (i == 0 ? (((ITEMS_BY_P * P) / C) + ((ITEMS_BY_P * P) % C)) : ((ITEMS_BY_P * P) / C));
        if (pthread_create(&threads_consumidores[i], NULL, &consumidor, (void *)&num_elementos_consumidores[i]))
        { // Hubo error, aun asi no abortamos
            perror("Error en pthread_create()");
        }
        printf("Generado el consumidor %d\n", i);
    }

    // El hilo principal espera a que acaben los demas
    for (i = 0; i < P; i++)
    {
        if (pthread_join(threads_productores[i], NULL))
        {
            perror("Error en pthread_join()");
        }
    }
    for (i = 0; i < C; i++)
    {
        if (pthread_join(threads_consumidores[i], NULL))
        {
            perror("Error en pthread_join()");
        }
    }

    pthread_cond_destroy(&condp);
    pthread_cond_destroy(&condc);
    pthread_mutex_destroy(&the_mutex);

    exit(EXIT_SUCCESS);
}
