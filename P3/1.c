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

int *buffer;                                       // El buffer compartido de memoria
int *cuenta;                                       // El numero de elementos que hay en el buffer, tambien es una variable compartida
int *cuenta_pids;                                  // El numero de elementos que hay en la lista de procesos (es decir, el numero de procesos que estan participando). Tambien es una variable compartida
int fd_cuenta, fd_buffer, fd_pids, fd_cuenta_pids; // Descriptores de archivo de los objetos anonimos compartidos en memoria, que se crean con shm_open()
int pids_mapeados = 0;                             // Puede ser que no tengamos mapeadas tantas entradas de la lista de procesos como hay actualmente, esta variable guarda las que hay mapeadas en este proceso para compararlas con las que hay en total (*cuenta_pids)
struct stat fd_pids_info;                          // Informacion sobre el tamano del objeto anonimo en memoria donde se guarda la lista de procesos actuales

typedef struct
{
    enum
    {
        CONSUMIDOR,
        PRODUCTOR,
        INACTIVO
    } tipo;
    pid_t pid;
} info_proceso; // Este struct es una entrada de la lista de procesos. Contiene su tipo (consumidor, productor, o si ya no es un proceso activo (es decir, ha acabado sus tareas)), y el PID del proceso.

info_proceso *vector_info_procesos; // La direccion donde tenemos mapeada la lista compartida de procesos

// Imprime por pantalla una representacion grafica del buffer. La parte ocupada de la lista se imprime en rojo, la parte libre en azul.
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
            printf(ANSI_COLOR_BLUE " %-3d " ANSI_COLOR_RESET "|", buffer[i]);
        }
    }
    printf("\n\t-");
    for (i = 0; i < TAM_BUFFER; i++)
    {
        printf("------");
    }
    printf("\n");
}

// Funcion sleep() de Tanenbaum. El proceso se envia a si mismo una senal SIGSTOP
void dormir()
{
    kill(getpid(), SIGSTOP);
}

// Funcion wakeup() de Tanenbaum. Le envia al proceso especificado la senal SIGCONT
void despertar(pid_t pid)
{
    kill(pid, SIGCONT);
}

// Actualiza el tamano del mapeo de memoria de la lista de procesos, para que se ajuste al tamano de la lista en si, ya que podria ir creciendo con el tiempo
void actualizar_mapeo_lista_procesos()
{
    if ((*cuenta_pids) > pids_mapeados)
    {
        if (pids_mapeados > 0) // Si ya habiamos hecho el mapeo de memoria de la lista de procesos, lo desmapeamos antes de nada. Tambien podriamos comprobar el valor del puntero vector_info_procesos.
        {
            if (munmap(vector_info_procesos, sizeof(info_proceso) * pids_mapeados))
            {
                perror("Error en munmap()");
                exit(EXIT_FAILURE);
            }
        }

        // Actualizamos el numero de entradas mapeadas de la lista
        pids_mapeados = *cuenta_pids;
        vector_info_procesos = (info_proceso *)mmap(NULL, pids_mapeados * sizeof(info_proceso), PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_pids, 0); // Mapeamos la lista compartida en memoria
        if (vector_info_procesos == MAP_FAILED)
        {
            perror("Error en mmap()");
            exit(EXIT_FAILURE);
        }
    }
}

// Devuelve el PID del primer productor vivo
pid_t get_pid_productor()
{
    int i;
    info_proceso *puntero;
    actualizar_mapeo_lista_procesos();                                                                // Si la lista de procesos ha crecido, actualizamos su tamano
    for (puntero = vector_info_procesos; puntero < (vector_info_procesos + pids_mapeados); puntero++) // Recorremos hasta el limite del mapeo local, asi que esta funcion no es vulnerable a carreras criticas (si la lista se expande en medio de esta funcion, no pasa nada, ya que no recorremos la parte nueva)
    {
        if (puntero->tipo == PRODUCTOR && puntero->pid != getpid())
        {
            return puntero->pid;
        }
    }
    return 0;
}

// Devuelve el PID del primer consumidor vivo
pid_t get_pid_consumidor()
{
    int i;
    info_proceso *puntero;
    actualizar_mapeo_lista_procesos();                                                                // Si la lista de procesos ha crecido, actualizamos su tamano
    for (puntero = vector_info_procesos; puntero < (vector_info_procesos + pids_mapeados); puntero++) // Recorremos hasta el limite del mapeo local, asi que esta funcion no es vulnerable a carreras criticas (si la lista se expande en medio de esta funcion, no pasa nada, ya que no recorremos la parte nueva)
    {
        if (puntero->tipo == CONSUMIDOR && puntero->pid != getpid())
        {
            return puntero->pid;
        }
    }
    return 0;
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
void productor()
{
    int num_elementos_restantes; // El numero de elementos que faltan por producir
    int item;                    // El item que estamos produciendo actualmente
    int cuenta_intermedio;

    for (num_elementos_restantes = 0; num_elementos_restantes < NUM_ELEMENTOS_TOTALES; num_elementos_restantes++)
    {
        item = producir();
        if (*cuenta == TAM_BUFFER)
        {
            dormir();
        }
        insertar_item_buffer(item);
        printf("(P) Inserto: %d\n", item);
        cuenta_intermedio = (*cuenta) + 1;
        sleep(1);
        (*cuenta) = cuenta_intermedio;
        imprimir_buffer();
        if (*cuenta == 1)
        {
            printf("(P) Voy a despertar a C\n");
            despertar(get_pid_consumidor());
        }
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
        if (*cuenta == 0)
        {
            dormir();
        }
        item = sacar_item_buffer();
        printf("(C) Saco: %d\n", item);
        cuenta_intermedio = (*cuenta) - 1;
        sleep(1);
        (*cuenta) = cuenta_intermedio;
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

    printf("MI PID: %d\n", getpid()); // Imprimimos el PID de este proceso por pantalla

    // Comprobamos si se ha invocado correctamente al programa
    if (argc != 2 || (strncmp("-c", argv[1], 2) != 0 && strncmp("-p", argv[1], 2) != 0))
    {
        fprintf(stderr, "Formato: %s [-c, para consumidor; -p, para productor]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    srand(clock()); // Semilla del generador aleatorio de numeros

    // Las funciones shm_open() crean un objeto compartido de memoria, que funciona como una especie de fichero que solo esta en memoria principal. Es decir, nunca llegara a tranferirse a memoria secundaria. Las usamos para compartir variables y regiones de memoria entre los distintos procesos de esta aplicacion.
    fd_cuenta = shm_open(NOMBRE_OBJETO_CUENTA, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_buffer = shm_open(NOMBRE_OBJETO_BUFFER, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_pids = shm_open(NOMBRE_OBJETO_PIDS, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fd_cuenta_pids = shm_open(NOMBRE_OBJETO_CUENTA_PIDS, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (!fd_buffer || !fd_cuenta || !fd_pids || !fd_cuenta_pids)
    {
        perror("Error en shm_open()");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd_cuenta, &fd_cuenta_info)) // Leemos la informacion sobre fd_cuenta
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    if (fd_cuenta_info.st_size == 0) // Si fd_cuenta no tiene tamano, es que lo estamos creando en este proceso. El resto de objetos tambien seran creados.
    {
        if (ftruncate(fd_cuenta, sizeof(int)) == -1) // Si el objeto originalmente no tenia el tamano para albergar un int, le ponemos nosotros ese tamano. De forma automatica, se escribe un 0.
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

    (*cuenta_pids)++; // Anadimos un proceso a la lista

    if (fstat(fd_pids, &fd_pids_info))
    {
        perror("Error en fstat()");
        exit(EXIT_FAILURE);
    }
    // Si el tamano del vector de structs info_proceso no es suficiente, lo agrandamos
    if ((*cuenta_pids) * sizeof(info_proceso) > fd_pids_info.st_size)
    {
        if (ftruncate(fd_pids, (*cuenta_pids) * sizeof(info_proceso)) == -1) // Anadimos sitio para un struct de informacion mas
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

    actualizar_mapeo_lista_procesos();

    if (cuenta == MAP_FAILED || buffer == MAP_FAILED || cuenta_pids == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    vector_info_procesos[*cuenta_pids - 1].pid = getpid(); // Escribimos la informacion de este proceso en la lista compartida
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

    if (munmap(buffer, sizeof(int) * TAM_BUFFER)) // Deshacemos los mapeos de memoria
    {
        perror("Error en munmap()");
        // No salimos con EXIT_FAILURE, ya que nos compensa seguir intentando desmapear el resto de memoria
    }
    if (munmap(cuenta, sizeof(int)))
    {
        perror("Error en munmap()");
    }
    if (munmap(vector_info_procesos, sizeof(info_proceso) * pids_mapeados))
    {
        perror("Error en munmap()");
    }
    if (munmap(cuenta_pids, sizeof(int)))
    {
        perror("Error en munmap()");
    }

    if (shm_unlink(NOMBRE_OBJETO_CUENTA)) // No se eliminara el objeto hasta que todos los procesos lo hayan cerrado
    {
        perror("Error en shm_unlink()");
    }
    if (shm_unlink(NOMBRE_OBJETO_BUFFER))
    {
        perror("Error en shm_unlink()");
    }
    if (shm_unlink(NOMBRE_OBJETO_PIDS))
    {
        perror("Error en shm_unlink()");
    }
    if (shm_unlink(NOMBRE_OBJETO_CUENTA_PIDS))
    {
        perror("Error en shm_unlink()");
    }

    exit(EXIT_SUCCESS);
}