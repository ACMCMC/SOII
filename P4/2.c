#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define P 5              // productores
#define C 4              // consumidores
#define N 5              // tamano del buffer
#define ITEMS_BY_P 10    // items por cada productor
#define SLEEP_MAX_TIME 4 // maximo tiempo para producir / consumir

int buffer[N]; // El buffer compartido de memoria
int cuenta;    // El numero de elementos de la cola
int primero;   // El lugar donde esta el primero de la cola

pthread_mutex_t the_mutex;
sem_t *empty, *full;

/*====================================================================

COMENTARIOS A LA PRACTICA

He decidido implementar el control de las variables de condicion usando semaforos, como en practicas anteriores.
Usando solo mutexes, no he conseguido encontrar ninguna posibilidad que garantice que se asegure la correccion del algoritmo.

- Si, por ejemplo, implementase la espera que se hace sobre una variable de condicion (cuando en el codigo anterior hacia pthread_cond_wait(&condp, &the_mutex)), con un nuevo mutex (llamemoslo mutex_empty), entonces tendria que soltar the_mutex y hacer que se bloquee el proceso en base al nuevo mutex. Pero para eso, necesitaria que algun otro proceso ya hubiese adquirido el mutex, y esto no se me ocurre una forma de garantizarlo. Ademas, aun consiguiendo implementarlo, tendria el problema de que no tengo garantizado que mi proceso se ejecute todo el tiempo (es decir, la propiedad de atomicidad), entre una llamada a pthread_mutex_unlock(the_mutex) y otra a pthread_mutex_lock(mutex_empty). Asi que podria pasar que entre una llamada y otra, llegue otro productor y inserte items en el buffer, y entonces ya no fuese pertinente bloquear al proceso para que espere (y esto es logico porque hemos desbloqueado the_mutex, asi que puede haber carreras criticas). Pero si soltamos el mutex, entonces precisamente estamos perdiendo la exclusion mutua, y la necesitamos para asegurar que el acceso al buffer sea atomico. Pero la otra opcion seria incluir otro mutex para asegurar que esa parte se hace de forma atomica, y seria aun peor, porque podria dar lugar a interbloqueos. No se me ha ocurrido otra forma de hacerlo exclusivamente con mutexes. 

====================================================================*/

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
        if (sem_wait(empty))
        {
            perror("Error en sem_wait");
        }
        pthread_mutex_lock(&the_mutex);
        printf("(C) Adquiero el mutex\n");
        insertar_item_buffer(item);
        printf("(P) Inserto: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        pthread_mutex_unlock(&the_mutex);
        if (sem_post(full))
        {
            perror("Error en sem_post");
        }
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
        if (sem_wait(full))
        {
            perror("Error en sem_wait");
        }
        pthread_mutex_lock(&the_mutex);
        printf("(C) Adquiero el mutex\n");
        item = sacar_item_buffer();
        printf("(C) Saco: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        pthread_mutex_unlock(&the_mutex);
        if (sem_post(empty))
        {
            perror("Error en sem_post");
        }
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

    if (pthread_mutex_init(&the_mutex, 0))
    {
        perror("Error en pthread_mutex_init()");
        exit(EXIT_FAILURE);
    }
    empty = sem_open("/empty", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, N);
    full = sem_open("/full", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (empty == SEM_FAILED || full == SEM_FAILED)
    {
        perror("Error en sem_open()");
        exit(EXIT_FAILURE);
    }

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

    pthread_mutex_destroy(&the_mutex);
    if (sem_close(empty))
    {
        perror("Error en sem_close");
    }
    if (sem_close(full))
    {
        perror("Error en sem_close");
    }

    if (sem_unlink("/empty"))
    {
        perror("Error en sem_unlink()");
    }
    if (sem_unlink("/full"))
    {
        perror("Error en sem_unlink()");
    }

    exit(EXIT_SUCCESS);
}
