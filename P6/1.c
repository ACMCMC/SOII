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
    El problema de los filosofos es el problema mas clasico que hay a la hora
    de abordar los problemas de sincronizacion entre procesos e hilos. En este
    caso, se trata de una serie de hilos que deben adquirir atomicamente dos
    recursos a la vez, y la dificultad se encuentra en garantizar que no se
    produzcan carreras criticas por el acceso a esos recursos, y al mismo
    que se permite el acceso (que no exista inanicion).

    El codigo que he escrito es muy similar al del Tanenbaum, adaptado a las
    funciones adecuadas de POSIX. No es muy largo, pero sí tiene una cierta
    complejidad conceptual, especialmente a la hora de pensar en la concurrencia
    de los distintos filosofos que tratan de obtener los tenedores a la vez.
    En mi opinion, la funcion probar() merece un comentario aparte, que he
    incluido mas abajo.

    Para garantizar exclusion mutua en el proceso de adquisicion y liberacion
    de recursos (tomar y poner los tenedores), se usa un semaforo a modo de
    mutex. Ademas, se usa un semaforo por cada filosofo, para que se bloquee
    si no puede comer (esto lo explico en la funcion probar).

    Como observacion adicional, cabe indicar que no he incluido comprobaciones
    de error en todas las llamadas al sistema. Esto lo hago para facilitar la
    lectura del codigo, y que se parezca mas al recogido en el Tanenbaum.
================================================================================*/

sem_t mutex;                 // Semaforo que evita la aparicion de carreras criticas
int estado[N];               // Los estados de los N filosofos
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

void pensar()
{
    sleep(((int)rand()) % (MAX_TIEMPO_PENSAR - 1) + 1); // Introducimos una espera aleatoria
}

void comer()
{
    sleep(((int)rand()) % (MAX_TIEMPO_COMER - 1) + 1); // Introducimos una espera aleatoria
}

/*
La funcion probar() tiene tres usos, y un unico objetivo. El objetivo es probar
si el filosofo que se le pasa como parametro puede comer. Si puede comer,
entonces se ejecuta un up() de su semaforo. El primer uso que se le da a esta
funcion es para que un filosofo "se compruebe a si mismo". Si no puede comer,
no se habra ejecutado un up(), y ya que tras llamar a esta funcion se ejecuta
un down(), el filosofo que llama a la funcion se quedara bloqueado. El segundo
uso que se le da, es que cuando el filosofo deje sus tenedores, ejecutara esta
funcion pero pasandole como argumento al filosofo de su izquierda. Por tanto,
esta funcion ejecutara un up() del semaforo de la izquierda, si ese filosofo
esta hambriento (no hace nada si esta pensando, y es imposible que este
comiendo), y los dos que tiene al lado no estan comiendo. Ademas, pone su
estado como "COMIENDO". El tercer uso es identico al segundo, pero con el de
la derecha. De esta forma, aseguramos que nadie se muere de inanicion (los
filosofos hambrientos son despertados cuando pueden comer), y que tampoco se
producen interbloqueos (ya que los tenedores se adquieren de forma atomica).
*/
void probar(int num_filosofo)
{
    // Si el filosofo que nos pasan como argumento esta hambriento y sus vecinos no estan comiendo...
    if (estado[num_filosofo] == HAMBRIENTO && estado[IZQUIERDO] != COMIENDO && estado[DERECHO] != COMIENDO)
    {
        estado[num_filosofo] = COMIENDO;             // Entonces este filosofo puede comer
        sem_post(&semaforosFilosofos[num_filosofo]); // Y ejecutamos un up() de su semaforo
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

// Funcion del Tanenbaum
void *filosofo(void *p_num_filosofo)
{
    int num_filosofo = ((int *)p_num_filosofo); // Recuperamos el numero de este filosofo

    while (1) // Bucle infinito
    {
        pensar();                      // Empezamos pensando durante un tiempo aleatorio
        tomar_tenedores(num_filosofo); // Este filosofo va a intentar adquirir 2 tenedores. Si no puede, se quedara bloqueado en el camino
        comer();                       // En este punto, este filosofo tiene dos tenedores de forma segura, asi que puede comer
        poner_tenedores(num_filosofo); // Este filosofo se encarga de devolver los tenedores a la mesa
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads_filosofos[N];
    int i;

    srand(clock()); // Semilla del generador aleatorio de numeros

    if (sem_init(&mutex, 0, 1))
    { // Inicializamos un semaforo anonimo, compartido por todos los hilos (se indica con el segundo argumento = 0), e inicializado a 1
        perror("Error en sem_init()");
    }

    // Genero los semaforos de los N filosofos. Esto no lo hago conjuntamente con el bucle siguiente, que se encarga de crear los hilos, porque un filosofo accede al semaforo de otro filosofo en la funcion probar(). Es decir, creo que es conveniente que antes de ejecutar el codigo de cualquier filosofo, esten creados los semaforos de todos ellos. Pensemos en un caso extremo, en el que un filosofo obtuviese el uso de la CPU mientras su vecino aun no esta creado, dejase de pensar, consiguiese comer, y al dejar los tenedores ejecutase la funcion probar() sobre un vecino suyo que no este creado. Esto no seria un problema, porque por defecto el estado de un filosofo no es "HAMBRIENTO", asi que nunca se ejecutaria un up() de un vecino que no existe, pero me parece mas elegante hacer las inicializaciones separadamente.
    for (i = 0; i < N; i++)
    {
        if (sem_init(&semaforosFilosofos, 0, 0))
        { // Inicializamos el semaforo de cada filosofo a 0
            perror("Error en sem_init()");
            exit(EXIT_FAILURE);
        }
    }

    // Genero los hilos de los N filosofos
    for (i = 0; i < N; i++)
    {
        if (pthread_create(&threads_filosofos[i], NULL, &filosofo, (void *)(i))) // Creamos un hilo por cada filosofo. El argumento que le pasamos es su propio numero.
        {
            perror("Error en pthread_create()");
            exit(EXIT_FAILURE);
        }
        printf("Generado el filosofo %d\n", i);
    }

    // El codigo escrito a partir de aqui nunca se llegara a ejecutar, porque he indicado que los filosofos intenten comer infinitamente.

    // El hilo principal espera a que acaben los demas
    for (i = 0; i < N; i++)
    {
        if (pthread_join(threads_filosofos[i], NULL))
        {
            perror("Error en pthread_join()");
            exit(EXIT_FAILURE);
        }
    }

    // Destruyo los semaforos de los N filosofos. Esto no lo hago conjuntamente con el bucle anterior, por un motivo analogo al que expuse antes. Si un filosofo acaba y destruyo su semaforo, podria darse el caso de que aun asi otro filosofo que siguiera vivo llamase a probar() sobre el filosofo destruido, y aunque nunca se ejecutaria un up() del semaforo, ya que un filosofo no deberia acabar en condiciones normales su ejecucion mientras esta hambriento, creo que es mas elegante hacer la destruccion cuando ya sabemos que no queda ningun filosofo vivo.
    for (i = 0; i < N; i++)
    {
        if (sem_init(&semaforosFilosofos, 0, 0))
        { // Inicializamos el semaforo de cada filosofo a 0
            perror("Error en sem_init()");
            exit(EXIT_FAILURE);
        }
    }

    sem_destroy(&mutex); // Una vez finalizados todos los hilos de los filosofos, destruimos el mutex

    exit(EXIT_SUCCESS);
}
