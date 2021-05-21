#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

// Codigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define N 8                                  // Numero de filosofos
#define IZQUIERDO (num_filosofo + N - 1) % N // El vecino izquierdo de i
#define DERECHO (num_filosofo + 1) % N       // El vecino derecho de i
#define PENSANDO 0
#define HAMBRIENTO 1
#define COMIENDO 2
#define MAX_TIEMPO_PENSAR 5 // El tiempo maximo que un filosofo esta pensando
#define MAX_TIEMPO_COMER 5  // El tiempo maximo que un filosofo esta comiendo

/*================================================================================
  .-.            ___                                  .-.                          
 /    \    .-.  (   )                                /    \                        
 | .`. ;  ( __)  | |    .--.       .--.      .--.    | .`. ;    .--.       .--.    
 | |(___) (''")  | |   /    \    /  _  \    /    \   | |(___)  /    \    /  _  \   
 | |_      | |   | |  |  .-. ;  . .' `. ;  |  .-. ;  | |_     |  .-. ;  . .' `. ;  
(   __)    | |   | |  | |  | |  | '   | |  | |  | | (   __)   | |  | |  | '   | |  
 | |       | |   | |  | |  | |  _\_`.(___) | |  | |  | |      | |  | |  _\_`.(___) 
 | |       | |   | |  | |  | | (   ). '.   | |  | |  | |      | |  | | (   ). '.   
 | |       | |   | |  | '  | |  | |  `\ |  | '  | |  | |      | '  | |  | |  `\ |  
 | |       | |   | |  '  `-' /  ; '._,' '  '  `-' /  | |      '  `-' /  ; '._,' '  
(___)     (___) (___)  `.__.'    '.___.'    `.__.'  (___)      `.__.'    '.___.'   
                                                                                   
                                                                                   
                                                                             
Aldan Creo Marino, SOII 2020/21

Comentario al codigo:
    Como observacion adicional, cabe indicar que no he incluido comprobaciones
    de error en todas las llamadas al sistema. Esto lo hago para facilitar la
    lectura del codigo, y que se parezca mas al recogido en el Tanenbaum.
================================================================================*/

sem_t mutex;                 // Semaforo que evita la aparicion de carreras criticas
int estado[N];               // Los estados de los N filosofos
int cuentaSillas;            // El numero de sillas ocupadas
sem_t semaforosFilosofos[N]; // Un semaforo por cada filosofo

// Funcion auxiliar que imprime la mesa
void imprimir_mesa()
{
    int i;
    printf("\t");
    for (i = 0; i < N; i++)
    {
        printf("-----");
    }
    printf("\n\t");
    for (i = 0; i < N; i++)
    {
        if (estado[i] == COMIENDO)
        {
            printf(ANSI_COLOR_RED " (C) " ANSI_COLOR_RESET);
        }
        else if (estado[i] == HAMBRIENTO)
        {
            printf(ANSI_COLOR_CYAN " (H) " ANSI_COLOR_RESET);
        }
        else
        {
            printf(ANSI_COLOR_GREEN " (P) " ANSI_COLOR_RESET);
        }
    }
    printf("\n\t");
    for (i = 0; i < N; i++)
    {
        printf("-----");
    }
    printf("\n\n");
}

// Espera entre 1 y 30 segundos
void pensar()
{
    sleep(((int)rand()) % (MAX_TIEMPO_PENSAR - 1) + 1); // Introducimos una espera aleatoria
}

// Espera 4 segundos
void comer()
{
    sleep(((int)rand()) % (MAX_TIEMPO_COMER - 1) + 1); // Introducimos una espera aleatoria
}

void probar(int num_filosofo)
{
    if (estado[num_filosofo] == HAMBRIENTO && estado[IZQUIERDO] != COMIENDO && estado[DERECHO] != COMIENDO)
    {
        estado[num_filosofo] = COMIENDO;
        sem_post(&semaforosFilosofos[num_filosofo]);
    }
}

void tomar_tenedores(int num_filosofo)
{
    sem_wait(&mutex);                  // Adquirimos el mutex
    estado[num_filosofo] = HAMBRIENTO; // Indicamos que estamos hambrientos
    printf("[%d] HAMBRIENTO\n", num_filosofo);
    imprimir_mesa();
    probar(num_filosofo);                        // Funcion probar()
    sem_post(&mutex);                            // Liberamos el mutex
    sem_wait(&semaforosFilosofos[num_filosofo]); // Si no podemos comer, nos quedamos bloqueados. Si hemos conseguido ambos tenedores, entonces podremos continuar
}

void poner_tenedores(int num_filosofo)
{
    sem_wait(&mutex);                // Adquirimos el mutex
    estado[num_filosofo] = PENSANDO; // Hemos dejado de comer
    probar(IZQUIERDO);               // Probamos si el vecino de la izquierda puede comer
    probar(DERECHO);                 // Probamos si el vecino de la derecha puede comer
    printf("[%d] PENSANDO\n", num_filosofo);
    imprimir_mesa();
    sem_post(&mutex); // Liberamos el mutex
}

void *filosofo(void *p_num_filosofo)
{
    int num_filosofo = ((int *)p_num_filosofo);

    while (1)
    {
        pensar();
        tomar_tenedores(num_filosofo);
        comer();
        poner_tenedores(num_filosofo);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads_filosofos[N];
    int i;

    srand(clock()); // Semilla del generador aleatorio de numeros

    sem_init(&mutex, 0, 1); // Inicializamos un semaforo anonimo, compartido por todos los hilos (se indica con el segundo argumento = 0), e inicializado a 1

    // Generamos los hilos de los N filosofos
    for (i = 0; i < N; i++)
    {
        if (sem_init(&semaforosFilosofos, 0, 0))
        { // Inicializamos el semaforo de cada filosofo a 0
            perror("Error en sem_init()");
            exit(EXIT_FAILURE);
        }
        if (pthread_create(&threads_filosofos[i], NULL, &filosofo, (void *)(i))) // Creamos un hilo por cada filosofo. El argumento que le pasamos es su propio numero.
        {
            perror("Error en pthread_create()");
            exit(EXIT_FAILURE);
        }
        printf("Generado el filosofo %d\n", i);
    }

    // El hilo principal espera a que acaben los demas
    for (i = 0; i < N; i++)
    {
        if (pthread_join(threads_filosofos[i], NULL))
        {
            perror("Error en pthread_join()");
            exit(EXIT_FAILURE);
        }
        sem_destroy(&semaforosFilosofos[i]);
    }

    sem_destroy(&mutex);

    exit(EXIT_SUCCESS);
}
