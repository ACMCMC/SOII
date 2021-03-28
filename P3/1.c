#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAX_SLEEP_PRODUCTOR 2
#define MAX_SLEEP_CONSUMIDOR 4
#define NUM_PRODS 1
#define NUM_CONS 1
#define NUM_ELEMENTOS_TOTALES 30
#define TAM_BUFFER 8
#define NOMBRE_FICHERO_BUFFER "buffer"
#define NOMBRE_FICHERO_PIDS "pids"
#define NOMBRE_FICHERO_CUENTA "cuenta"
#define NOMBRE_FICHERO_CUENTA_PIDS "cuenta_pids"

// Códigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int *buffer, *cuenta, *cuenta_pids;
int fd_cuenta, fd_buffer, fd_pids, fd_cuenta_pids;
struct stat fd_pids_info;

typedef struct
{
    enum
    {
        CONSUMIDOR,
        PRODUCTOR,
        INACTIVO
    } tipo;
    pid_t pid;
} info_proceso;

info_proceso *vector_info_procesos;

void imprimir_buffer()
{
    int i;
    printf("\n\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n\t|");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        if (i < *cuenta)
        {
            printf(ANSI_COLOR_RED " %-3d " ANSI_COLOR_RESET "|", buffer[i]);
        }
        else
        {
            printf(" N/A |");
        }
    }
    printf("\n\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n");
}

void dormir()
{
    kill(getpid(), SIGSTOP);
}

void despertar(pid_t pid)
{
    kill(pid, SIGCONT);
}

pid_t get_pid_productor()
{
    int i;
    info_proceso *puntero;
    for (puntero = vector_info_procesos; puntero < (vector_info_procesos + *cuenta_pids); puntero++)
    {
        if (puntero->tipo == PRODUCTOR && puntero->pid != getpid())
        {
            return puntero->pid;
        }
    }
    return 0;
}

pid_t get_pid_consumidor()
{
    int i;
    info_proceso *puntero;
    for (puntero = vector_info_procesos; puntero < (vector_info_procesos + *cuenta_pids); puntero++)
    {
        if (puntero->tipo == CONSUMIDOR && puntero->pid != getpid())
        {
            return puntero->pid;
        }
    }
    return 0;
}

void insertar_item_buffer(int item)
{
    buffer[*cuenta] = item;
}

int sacar_item_buffer()
{
    return buffer[*cuenta - 1];
}

int producir()
{
    sleep(((int)rand()) % MAX_SLEEP_PRODUCTOR);
    return ((int)rand() % 1000);
}

void consumir(int numero)
{
    sleep(((int)rand()) % MAX_SLEEP_CONSUMIDOR);
}

void productor()
{
    int num_elementos_restantes;
    int item;

    for (num_elementos_restantes = 0; num_elementos_restantes < NUM_ELEMENTOS_TOTALES; num_elementos_restantes++)
    {
        item = producir();
        if (*cuenta == TAM_BUFFER)
        {
            dormir();
        }
        insertar_item_buffer(item);
        (*cuenta)++;
        imprimir_buffer();
        if (*cuenta == 1)
        {
            printf("(P) Voy a despertar a C\n");
            despertar(get_pid_consumidor());
        }
    }
    printf("(P) He acabado!\n");
}

void consumidor()
{
    int num_elementos_restantes;
    int item;

    for (num_elementos_restantes = 0; num_elementos_restantes < NUM_ELEMENTOS_TOTALES; num_elementos_restantes++)
    {
        if (*cuenta == 0)
        {
            dormir();
        }
        item = sacar_item_buffer();
        (*cuenta)--;
        imprimir_buffer();
        if (*cuenta == (TAM_BUFFER - 1))
        {
            printf("(C) Voy a despertar a P\n");
            despertar(get_pid_productor());
        }
        consumir(item);
    }
    printf("(C) He acabado!\n");
}

int main(int argc, char **argv)
{
    struct stat fd_cuenta_info;
    printf("MI PID: %d\n", getpid());
    /*shm_unlink(NOMBRE_FICHERO_CUENTA); // No se eliminará el archivo hasta que todos los procesos lo hayan cerrado
    shm_unlink(NOMBRE_FICHERO_BUFFER);
    shm_unlink(NOMBRE_FICHERO_PIDS);
    shm_unlink(NOMBRE_FICHERO_CUENTA_PIDS);*/

    if (argc != 2 || (strncmp("-c", argv[1], 2) != 0 && strncmp("-p", argv[1], 2) != 0))
    {
        fprintf(stderr, "Formato: %s [-c, para consumidor; -p, para productor]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    srand(clock()); // Semilla del generador aleatorio de números

    fd_cuenta = shm_open(NOMBRE_FICHERO_CUENTA, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_buffer = shm_open(NOMBRE_FICHERO_BUFFER, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_pids = shm_open(NOMBRE_FICHERO_PIDS, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_cuenta_pids = shm_open(NOMBRE_FICHERO_CUENTA_PIDS, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (!fd_buffer || !fd_cuenta || !fd_pids || !fd_cuenta_pids)
    {
        perror("Error en shm_open()");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd_cuenta, &fd_cuenta_info))
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    if (fd_cuenta_info.st_size == 0)
    {                                                // Si el fichero de cuenta no tiene tamaño, es que lo estamos creando en este proceso. El resto de ficheros también serán creados.
        if (ftruncate(fd_cuenta, sizeof(int)) == -1) // Si el fichero originalmente no tenía el tamaño para albergar un int, le ponemos nosotros ese tamaño. De forma automática, se escribe un 0.
        {
            perror("Error en ftruncate() para fd_cuenta");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(fd_cuenta_pids, sizeof(int)) == -1)
        {
            perror("Error en ftruncate() para fd_cuenta_pids");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(fd_buffer, sizeof(int) * TAM_BUFFER) == -1)
        {
            perror("Error en ftruncate() para fd_buffer");
            exit(EXIT_FAILURE);
        }
    }

    // Mapeamos la memoria compartida entre los procesos
    cuenta = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_cuenta, 0);
    cuenta_pids = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_cuenta_pids, 0);
    buffer = (int *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_buffer, 0);

    (*cuenta_pids)++; // Añadimos un proceso a la lista

    if (fstat(fd_pids, &fd_pids_info))
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    if ((*cuenta_pids) * sizeof(info_proceso) > fd_pids_info.st_size)
    { // Si el tamaño del vector de structs info_proceso no es suficiente, lo agrandamos
        if (ftruncate(fd_pids, (*cuenta_pids) * sizeof(info_proceso)) == -1) // Añadimos sitio para un struct de información más
        {
            perror("Error en ftruncate() para fd_pids");
            exit(EXIT_FAILURE);
        }
        if (fstat(fd_pids, &fd_pids_info))
        {
            perror("Error en fstat()");
            exit(EXIT_FAILURE);
        }
    }

    vector_info_procesos = (info_proceso *)mmap(NULL, fd_pids_info.st_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_pids, 0);

    if (cuenta == MAP_FAILED || buffer == MAP_FAILED || vector_info_procesos == MAP_FAILED || cuenta_pids == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    vector_info_procesos[*cuenta_pids - 1].pid = getpid();
    vector_info_procesos[*cuenta_pids - 1].tipo = argv[1][1] == 'c' ? CONSUMIDOR : PRODUCTOR;

    if (argv[1][1] == 'c')
    {
        consumidor();
    }
    else
    {
        productor();
    }

    vector_info_procesos[*cuenta_pids - 1].tipo = INACTIVO;

    munmap(buffer, sizeof(int) * TAM_BUFFER); // Deshacemos los mapeos de memoria
    munmap(cuenta, sizeof(int));
    munmap(vector_info_procesos, fd_pids_info.st_size);
    munmap(cuenta_pids, sizeof(int));

    shm_unlink(NOMBRE_FICHERO_CUENTA); // No se eliminará el archivo hasta que todos los procesos lo hayan cerrado
    shm_unlink(NOMBRE_FICHERO_BUFFER);
    shm_unlink(NOMBRE_FICHERO_PIDS);
    shm_unlink(NOMBRE_FICHERO_CUENTA_PIDS);

    exit(EXIT_SUCCESS);
}