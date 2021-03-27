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
#define MAX_SLEEP_CONSUMIDOR 3
#define NUM_PRODS 1
#define NUM_CONS 1
#define NUM_ELEMENTOS_TOTALES 30
#define TAM_BUFFER 8

// Códigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int *buffer, *cuenta;

typedef struct
{
    enum
    {
        CONSUMIDOR,
        PRODUCTOR
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
    printf("(%c) Voy a dormirme\n", getpid() == pid_consumidor ? 'C' : 'P');
    kill(getpid(), SIGSTOP);
    printf("(%c) He despertado!\n", getpid() == pid_consumidor ? 'C' : 'P');
}

void despertar(pid_t pid)
{
    kill(pid, SIGCONT);
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
            despertar(pid_consumidor);
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
            despertar(pid_productor);
        }
        consumir(item);
    }
    printf("(C) He acabado!\n");
}

int main(int argc, char **argv)
{
    int fd_cuenta, fd_buffer, fd_pids;
    struct stat fd_pifs_info, fd_cuenta_info;

    if (argc != 2 || strncmp("-c", argv[1], 2) != 0 || strncmp("-p", argv[1], 2) != 0)
    {
        perror("Formato: %s [-c, para consumidor; -p, para productor]");
        exit(EXIT_SUCCESS);
    }

    srand(clock()); // Semilla del generador aleatorio de números

    fd_cuenta = shm_open("cuenta", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_buffer = shm_open("buffer", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_pids = shm_open("pids", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (!fd_buffer || !fd_cuenta || !fd_pids)
    {
        perror("Error en shm_open()");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd_pids, &fd_pifs_info))
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    if (fstat(fd_cuenta, &fd_cuenta_info))
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }

    if (fd_cuenta_info.st_size != sizeof(int))
    { // Si el fichero originalmente no tenía el tamaño para albergar un int, le ponemos nosotros ese tamaño. El valor del tamaño que tenía al principio lo guardamos para después.
        if (ftruncate(fd_cuenta, sizeof(int)) == -1)
        {
            perror("Error en ftruncate()");
            exit(EXIT_FAILURE);
        }
    }
    if (ftruncate(fd_buffer, sizeof(int) * TAM_BUFFER) == -1)
    {
        perror("Error en ftruncate()");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd_pids, fd_pifs_info.st_size + sizeof(info_proceso)) == -1) // Añadimos sitio para un struct de información más
    {
        perror("Error en ftruncate()");
        exit(EXIT_FAILURE);
    }

    // Mapeamos la memoria compartida entre los procesos
    cuenta = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_cuenta, 0);
    buffer = (int *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_buffer, 0);
    vector_info_procesos = (info_proceso *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_pids, 0);

    if (cuenta == MAP_FAILED || buffer == MAP_FAILED || vector_info_procesos == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    if (fd_cuenta_info.st_size != sizeof(int))
    { // Si el fichero originalmente no tenía el tamaño para albergar un int, entonces lo habremos expandido para eso, y ahora que lo hemos mapeado en memoria, escribiremos un 0 en él.
        *cuenta = 0;
    }

    if (argv[1][1] == 'c')
    {
        consumidor();
    }
    else
    {
        productor();
    }

    munmap(buffer, sizeof(int) * TAM_BUFFER);
    munmap(cuenta, sizeof(int));

    shm_unlink();

    exit(EXIT_SUCCESS);
}