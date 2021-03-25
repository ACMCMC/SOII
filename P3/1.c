#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

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

pid_t pid_productor, pid_consumidor;

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
    printf("(%c) Voy a dormirme\n", getpid()==pid_consumidor ? 'C' : 'P' );
    kill(getpid(), SIGSTOP);
    printf("(%c) He despertado!\n", getpid()==pid_consumidor ? 'C' : 'P' );
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

int main()
{
    pid_t pid;

    srand(clock()); // Semilla del generador aleatorio de números

    // Mapeamos la memoria compartida entre los procesos
    cuenta = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    buffer = (int *)mmap(NULL, sizeof(int) * TAM_BUFFER, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    *cuenta = 0;

    pid_productor = getpid();

    pid = fork();

    if (pid < 0)
    {
        perror("Error en fork()");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // Hijo. Este va a ser el consumidor.
        pid_consumidor = getpid();
        consumidor();
    }
    else
    { // Padre. Va a ser el productor.
        pid_consumidor = pid;
        productor();
    }

    munmap(buffer, sizeof(int) * TAM_BUFFER);
    munmap(cuenta, sizeof(int));
}