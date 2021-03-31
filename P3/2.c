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

#define MAX_SLEEP_PRODUCTOR 2
#define MAX_SLEEP_CONSUMIDOR 3
#define NUM_PRODS 1
#define NUM_CONS 1
#define NUM_ELEMENTOS_TOTALES 30
#define TAM_BUFFER 8
#define NOMBRE_OBJETO_BUFFER "buffer"
#define NOMBRE_OBJETO_PIDS "pids"
#define NOMBRE_OBJETO_CUENTA "cuenta"
#define NOMBRE_OBJETO_CUENTA_PIDS "cuenta_pids"

// Codigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int *buffer;   // El buffer compartido de memoria
int fd_buffer; // Descriptor de archivo del objetos anonimo compartido en memoria, que se crea con shm_open()

sem_t *empty, *mutex, *full, *mutex_contador_lista; // Variables semaforo

// Imprime por pantalla una representacion grafica del buffer. La parte ocupada de la lista se imprime en rojo, la parte libre en azul.
void imprimir_buffer()
{
    int i, cuenta;
    sem_getvalue(full, &cuenta); // No garantiza que el valor este actualizado, pero no es una cuestion critica. Aparte, llamamos a esta funcion desde una zona que tiene exclusion mutua
    printf("\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n\t|");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        if (i < cuenta)
        {
            printf(ANSI_COLOR_RED " %-3d " ANSI_COLOR_RESET "|", buffer[i]);
        }
        else
        {
            printf(ANSI_COLOR_BLUE " %-3d " ANSI_COLOR_RESET "|", buffer[i]);
        }
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
    int cuenta;
    sem_getvalue(full, &cuenta); // No garantiza que el valor este actualizado, pero llamamos a esta funcion desde una zona que tiene exclusion mutua.
    printf("Esto va %d\n", cuenta);
    buffer[cuenta] = item;
}

// Saca un item del buffer y escribe un 0 en su lugar
int sacar_item_buffer()
{
    int cuenta;
    sem_getvalue(full, &cuenta); // No garantiza que el valor este actualizado, pero llamamos a esta funcion desde una zona que tiene exclusion mutua.
    printf("Esto va %d\n", cuenta);
    int elem = buffer[cuenta - 1];
    buffer[cuenta - 1] = 0;
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
void productor()
{
    int num_elementos_restantes; // El numero de elementos que faltan por producir
    int item;                    // El item que estamos produciendo actualmente
    int cuenta_intermedio;

    for (num_elementos_restantes = 0; num_elementos_restantes < NUM_ELEMENTOS_TOTALES; num_elementos_restantes++)
    {
        item = producir();
        sem_wait(empty);
        sem_wait(mutex);
        insertar_item_buffer(item);
        printf("(P) Inserto: %d\n", item);
        sleep(1);
        imprimir_buffer();
        sem_post(mutex);
        sem_post(full);
    }
    printf("(P) He acabado!\n");
}

// Implementa la funcion de las diapositivas
void consumidor()
{
    int num_elementos_restantes; // El numero de elementos que faltan por consumir
    int item;                    // El item que estamos consumiendo actualmente
    int cuenta_intermedio;

    for (num_elementos_restantes = 0; num_elementos_restantes < NUM_ELEMENTOS_TOTALES; num_elementos_restantes++)
    {
        sem_wait(full);
        sem_wait(mutex);
        item = sacar_item_buffer();
        printf("(C) Saco: %d\n", item);
        sleep(1);
        imprimir_buffer();
        sem_post(mutex);
        sem_post(empty);
        consumir(item);
    }
    printf("(C) He acabado!\n");
}

int main(int argc, char **argv)
{
    struct stat fd_buffer_info;
    int semaforos_creados; // Flag para saber si hay que cerrar los semaforos (porque los crea este proceso)
    /*sem_unlink("/SOII/empty");
    sem_unlink("/SOII/full");
    sem_unlink("/SOII/mutex");
    sem_unlink("/SOII/mutex_contador_lista");*/
    /*if (shm_unlink(NOMBRE_OBJETO_BUFFER))
    {
        perror("Error en shm_unlink()");
    }*/

    printf("MI PID: %d\n", getpid()); // Imprimimos el PID de este proceso por pantalla

    // Comprobamos si se ha invocado correctamente al programa
    if (!(argc == 2 || (argc == 3 && (strncmp("-r", argv[2], 2) == 0))) || (strncmp("-c", argv[1], 2) != 0 && strncmp("-p", argv[1], 2) != 0))
    {
        fprintf(stderr, "Formato: %s [-c, para consumidor; -p, para productor] [-r, para reiniciar los semaforos]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    srand(clock()); // Semilla del generador aleatorio de numeros

    // Opcion de reiniciar los semaforos
    if (argc == 3 && (strncmp("-r", argv[2], 2) == 0)) {
        if (sem_unlink("/SOII/empty"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/full"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/mutex"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/mutex_contador_lista"))
        {
            perror("Error en sem_unlink()");
        }
    }

    // Generamos los semaforos. O_CREAT no tiene efecto si el semaforo ya estaba creado.
    mutex = sem_open("/SOII/mutex", 0);
    // Si la funcion falla, es porque los semaforos no estan creados, asi que los creamos
    if (mutex == SEM_FAILED)
    {
        semaforos_creados = 1;
        printf("Semaforos creados.\n");
        mutex = sem_open("/SOII/mutex", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); // Usamos O_CREAT | O_EXCL porque podria haber dos procesos que detectaran que los semaforos no existen al mismo tiempo. De esta forma, solo uno de ellos los creara y el otro saldra con codigo de error.
        mutex_contador_lista = sem_open("/SOII/mutex_contador_lista", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        empty = sem_open("/SOII/empty", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, TAM_BUFFER);
        full = sem_open("/SOII/full", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    }
    else
    {
        // Abrimos el resto de semaforos normalmente, ya que estaban creados de antes
        semaforos_creados = 0;
        mutex_contador_lista = sem_open("/SOII/mutex_contador_lista", 0);
        empty = sem_open("/SOII/empty", 0);
        full = sem_open("/SOII/full", 0);
    }

    if (empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED || mutex_contador_lista == SEM_FAILED)
    {
        perror("Error en sem_open()");
        exit(EXIT_FAILURE);
    }

    fd_buffer = shm_open(NOMBRE_OBJETO_BUFFER, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (!fd_buffer)
    {
        perror("Error en shm_open()");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd_buffer, &fd_buffer_info)) // Leemos la informacion sobre fd_cuenta
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    if (fd_buffer_info.st_size == 0) // Si fd_buffer no tiene tamano, es que lo estamos creando en este proceso.
    {
        if (ftruncate(fd_buffer, sizeof(int) * TAM_BUFFER) == -1)
        {
            perror("Error en ftruncate() para fd_buffer");
            exit(EXIT_FAILURE);
        }
    }

    // Mapeamos la memoria compartida entre los procesos
    buffer = (int *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_buffer, 0);

    if (buffer == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    if (argv[1][1] == 'c')
    {
        consumidor();
    }
    else
    {
        productor();
    }

    if (munmap(buffer, sizeof(int) * TAM_BUFFER)) // Deshacemos los mapeos de memoria
    {
        perror("Error en munmap()");
        // No salimos con EXIT_FAILURE, ya que nos compensa seguir intentando desmapear el resto de memoria
    }

    if (shm_unlink(NOMBRE_OBJETO_BUFFER))
    {
        perror("Error en shm_unlink()");
    }

    // Cerramos los semaforos. No se cerraran realmente hasta que todos los procesos hayan dejado de usarlos.
    if (semaforos_creados) // Si este proceso es el que crea los semaforos, entonces los desvincula tambien.
    {
        printf("Semaforos desvinculados.\n");
        if (sem_unlink("/SOII/empty"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/full"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/mutex"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/SOII/mutex_contador_lista"))
        {
            perror("Error en sem_unlink()");
        }
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
    if (sem_close(mutex_contador_lista))
    {
        perror("Error en sem_close");
    }

    exit(EXIT_SUCCESS);
}