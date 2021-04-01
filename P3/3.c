#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

#define MAX_SLEEP_PRODUCTOR 3
#define MAX_SLEEP_CONSUMIDOR 3
#define NUM_PRODUCTORES 4
#define NUM_CONSUMIDORES 8
#define NUM_ELEMENTOS_TOTALES 11
#define TAM_BUFFER 8

// Codigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int *buffer; // El buffer compartido de memoria
int *cuenta; // La variable cuenta

sem_t *empty, *mutex, *full, *num_procesos; // Variables semaforo

// Imprime por pantalla una representacion grafica del buffer. La parte ocupada de la lista se imprime en rojo, la parte libre en azul.
void imprimir_buffer()
{
    int i;
    printf("\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n\t|");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf(ANSI_COLOR_BLUE " %-3d " ANSI_COLOR_RESET "|", buffer[i]);
    }
    printf("\n\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n\n");
}

// Inserta un item en el buffer
void insertar_item_buffer(int item)
{
    buffer[*cuenta] = item;
}

// Saca un item del buffer y escribe un 0 en su lugar
int sacar_item_buffer()
{
    int elem = buffer[*cuenta - 1];
    buffer[*cuenta - 1] = 0;
    return elem;
}

// Produce un item, para introducirlo en el buffer
int producir()
{
    sleep(((int)rand()) % MAX_SLEEP_PRODUCTOR); // Introducimos una espera aleatoria entre 0 y MAX_SLEEP_PRODUCTOR
    return ((int)rand() % 999) + 1;             // Numeros entre 1 y 999
}

// Consume un item del buffer
void consumir(int numero)
{
    sleep(((int)rand()) % MAX_SLEEP_CONSUMIDOR); // Introducimos una espera aleatoria entre 0 y MAX_SLEEP_CONSUMIDOR
}

// Implementa la funcion de las diapositivas
void productor(int num_elementos)
{
    int num_elementos_restantes; // El numero de elementos que faltan por producir
    int item;                    // El item que estamos produciendo actualmente

    for (num_elementos_restantes = 0; num_elementos_restantes < num_elementos; num_elementos_restantes++)
    {
        item = producir();
        if (sem_wait(empty))
        {
            perror("Error en sem_wait");
        }
        if (sem_wait(mutex))
        {
            perror("Error en sem_wait");
        }
        printf("(C) Adquiero el mutex\n");
        insertar_item_buffer(item);
        (*cuenta)++;
        printf("(P) Inserto: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        if (sem_post(mutex))
        {
            perror("Error en sem_post");
        }
        if (sem_post(full))
        {
            perror("Error en sem_post");
        }
    }
    printf("(P) He acabado!\n");
}

// Implementa la funcion de las diapositivas
void consumidor(int num_elementos)
{
    int num_elementos_restantes; // El numero de elementos que faltan por consumir
    int item;                    // El item que estamos consumiendo actualmente

    for (num_elementos_restantes = 0; num_elementos_restantes < num_elementos; num_elementos_restantes++)
    {
        if (sem_wait(full))
        {
            perror("Error en sem_wait");
        }
        if (sem_wait(mutex))
        {
            perror("Error en sem_wait");
        }
        printf("(C) Adquiero el mutex\n");
        item = sacar_item_buffer();
        (*cuenta)--;
        printf("(C) Saco: %d\n", item);
        imprimir_buffer();
        printf("(C) Libero el mutex\n");
        if (sem_post(mutex))
        {
            perror("Error en sem_post");
        }
        if (sem_post(empty))
        {
            perror("Error en sem_post");
        }
        consumir(item);
    }
    printf("(C) He acabado!\n");
}

int main(int argc, char **argv)
{
    pid_t pid;
    int i;
    int valor_semaforo_num_procesos;

    printf("MI PID: %d\n", getpid()); // Imprimimos el PID de este proceso por pantalla

    // Comprobamos si se ha invocado correctamente al programa
    if (!(argc == 2 && (strncmp("-r", argv[1], 2) == 0)) && argc != 1)
    {
        fprintf(stderr, "Formato: %s [-r, para reiniciar]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    srand(clock()); // Semilla del generador aleatorio de numeros

    // Opcion de reiniciar
    if (argc == 2 && (strncmp("-r", argv[1], 2) == 0))
    {
        if (sem_unlink("/empty"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/full"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/mutex"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/num_procesos"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
    }

    // Generamos los semaforos. O_CREAT no tiene efecto si el semaforo ya estaba creado.
    mutex = sem_open("/mutex", 0);
    // Si la funcion falla, es porque los semaforos no estan creados, asi que los creamos
    if (mutex == SEM_FAILED)
    {
        printf("Semaforos creados.\n");
        mutex = sem_open("/mutex", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); // Usamos O_CREAT | O_EXCL porque podria haber dos procesos que detectaran que los semaforos no existen al mismo tiempo. De esta forma, solo uno de ellos los creara y el otro saldra con codigo de error.
        num_procesos = sem_open("/num_procesos", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
        empty = sem_open("/empty", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, TAM_BUFFER);
        full = sem_open("/full", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    }
    else
    {
        // Abrimos el resto de semaforos normalmente, ya que estaban creados de antes
        num_procesos = sem_open("/num_procesos", 0);
        empty = sem_open("/empty", 0);
        full = sem_open("/full", 0);
    }

    if (empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED || num_procesos == SEM_FAILED)
    {
        perror("Error en sem_open()");
        exit(EXIT_FAILURE);
    }

    // Mapeamos la memoria compartida para los procesos. Ahora no hace falta un objeto anonimo compartido en memoria, ya que los hijos heredaran este mapeo que hacemos localmente.
    buffer = (int *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    cuenta = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (buffer == MAP_FAILED || cuenta == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    if (sem_post(num_procesos)) // Incrementamos el semaforo que lleva la cuenta del numero de procesos
    {
        perror("Error en sem_post");
    }

    *cuenta = 0; // Inicializamos la variable cuenta a 0. Deberia venir ya inicializada, pero creo que es buena practica asegurarnos.

    for ((i = 0, pid = -1); i < NUM_PRODUCTORES && pid != 0; i++)
    {
        pid = fork();
        if (pid < 0)
        { // Hubo error, aun asi no abortamos
            perror("Error en fork()");
        }
        else if (pid == 0)
        {                               // Este es el hijo
            if (sem_post(num_procesos)) // Incrementamos el semaforo que lleva la cuenta del numero de procesos
            {
                perror("Error en sem_post");
            }
            productor(i==0 ? ((NUM_ELEMENTOS_TOTALES / NUM_PRODUCTORES) + (NUM_ELEMENTOS_TOTALES % NUM_PRODUCTORES)) : (NUM_ELEMENTOS_TOTALES / NUM_PRODUCTORES));
        }
        else
        {
            printf("Generado el productor %d\n", i);
        }
    }
    for ((i = 0); i < NUM_CONSUMIDORES && pid != 0; i++)
    {
        pid = fork();
        if (pid < 0)
        { // Hubo error, aun asi no abortamos
            perror("Error en fork()");
        }
        else if (pid == 0)
        {                               // Este es el hijo
            if (sem_post(num_procesos)) // Incrementamos el semaforo que lleva la cuenta del numero de procesos
            {
                perror("Error en sem_post");
            }
            consumidor(i==0 ? ((NUM_ELEMENTOS_TOTALES / NUM_CONSUMIDORES) + (NUM_ELEMENTOS_TOTALES % NUM_CONSUMIDORES)) : (NUM_ELEMENTOS_TOTALES / NUM_CONSUMIDORES));
        }
        else
        {
            printf("Generado el consumidor %d\n", i);
        }
    }

    if (sem_wait(num_procesos)) // Decrementamos el semaforo que lleva la cuenta del numero de procesos. Si este era el ultimo proceso, obtiene el uso exclusivo del semaforo (ahora vale 0)
    {
        perror("Error en sem_wait");
    }

    if (sem_getvalue(num_procesos, &valor_semaforo_num_procesos))
    {
        perror("Error en sem_getvalue");
    }

    if (munmap(buffer, sizeof(int) * TAM_BUFFER)) // Deshacemos los mapeos de memoria
    {
        perror("Error en munmap()");
        // No salimos con EXIT_FAILURE, ya que nos compensa seguir intentando desmapear el resto de memoria
    }
    if (munmap(cuenta, sizeof(int))) // Deshacemos los mapeos de memoria
    {
        perror("Error en munmap()");
    }

    if (sem_close(empty))
    {
        perror("Error en sem_close");
    }
    if (sem_close(full))
    {
        perror("Error en sem_close");
    }
    if (sem_close(mutex))
    {
        perror("Error en sem_close");
    }
    if (sem_close(num_procesos))
    {
        perror("Error en sem_close");
    }

    // Cerramos los semaforos. No se cerraran realmente hasta que todos los procesos hayan dejado de usarlos.
    if (valor_semaforo_num_procesos == 0 && pid == 0) // Si este proceso es el ultimo (y estamos seguros de que lo es, porque si vale 0 hemos adquirido el mutex sobre el semaforo), eliminamos los semaforos y el objeto compartido de memoria. Introduzco la condicion pid==0 porque debe cerrarlos uno de los procesos hijo, no el padre.
    {
        if (sem_unlink("/empty"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/full"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/mutex"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/num_procesos"))
        {
            perror("Error en sem_unlink()");
        }
        printf("Semaforos desvinculados.\n");
    }

    exit(EXIT_SUCCESS);
}
