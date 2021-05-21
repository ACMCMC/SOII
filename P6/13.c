#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#define N 5              // Numero de clientes
#define NUM_SILLAS_ESPERA 3 // El numero de sillas en la sala de espera
#define TIEMPO_ATENCION 4 // El tiempo que se tarda en atender a un cliente
#define TIEMPO_PASEO 30 // El tiempo que estan paseando los clientes como maximo

pthread_mutex_t the_mutex;
pthread_cond_t condp, condc;

int sillas[N]; // Las sillas de la sala de espera. Se representa como un buffer compartido de memoria
int cuentaSillas;    // El numero de sillas ocupadas
int primeraSilla;   // El lugar donde esta la primera silla ocupada

// Imprime por pantalla una representacion grafica de las sillas de la sala de espera
void imprimir_sala_espera()
{
    int i;
    printf("\t");
    for (i = 0; i < N; i++)
    {
        printf("--------");
    }
    printf("\n\t");
    for (i = 0; i < N; i++)
    {
        if (sillas[i]!=0) {
            printf("|(%-2d)|  ", sillas[i]);
        } else {
            printf("|(  )|  ");
        }
    }
    printf("\n\t");
    for (i = 0; i < N; i++)
    {
        printf("--------");
    }
    printf("\n\n");
}

// Espera entre 1 y 30 segundos
void pasear()
{
    sleep(((int)rand()) % (TIEMPO_PASEO - 1) + 1); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
}

// Espera 4 segundos
void atenderCliente()
{
    sleep(((int)rand()) % TIEMPO_ATENCION); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
}

// "Si entra un cliente en la barberia y todas las sillas están ocupadas, abandona la barberia, y lo vuelve a intentar pasado un tiempo aleatorio"
void volverAIntentarTrasTiempoAleatorio() {
    pasear();
}

// Indica si hay sillas libres en la sala de espera
int hay_sillas_libres_sala_espera() {
    return cuentaSillas < N;
}

// Sienta a un cliente en la primera silla libre de la sala de espera
void sentarse_en_silla_espera(int idCliente)
{
    sillas[(primeraSilla + cuentaSillas) % N] = idCliente;
    cuentaSillas++;
}

// Levanta a un cliente de su silla en la sala de espera, devuelve su ID
int levantarse_de_silla_espera()
{
    int idCliente = sillas[primeraSilla];
    sillas[primeraSilla] = 0;
    primeraSilla = (primeraSilla + 1) % N;
    cuentaSillas--;
    return idCliente;
}

void *barbero(void *p)
{
    int num_clientes_restantes;
    int cliente;

    for (num_clientes_restantes = N; num_clientes_restantes > 0; num_clientes_restantes--)
    {
        pthread_mutex_lock(&the_mutex);
        printf("(Barbero) Accedo a la sala de espera\n");
        while (cuentaSillas == 0)
        {
            printf("(Barbero) No hay nadie. Me voy a dormir\n");
            pthread_cond_wait(&condc, &the_mutex);
        }
        cliente = levantarse_de_silla_espera();
        printf("(Barbero) Atiendo al cliente %d\n", cliente);
        imprimir_sala_espera();
        printf("(Barbero) Salgo de la sala de espera\n");
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&the_mutex);
        atenderCliente();
    }
    printf("(Barbero) He acabado mi trabajo!\n");
    return NULL;
}

void *cliente(void *p_num_cliente)
{
    int num_cliente = ((int *)p_num_cliente);

    pasear();
    pthread_mutex_lock(&the_mutex);
    printf("(Cliente %d) Accedo a la sala de espera\n", num_cliente);
    while (!hay_sillas_libres_sala_espera())
    {
        pthread_cond_wait(&condp, &the_mutex);
    }
    sentarse_en_silla_espera(num_cliente);
    printf("(Cliente %d) Estoy esperando en la sala de espera\n", num_cliente);
    imprimir_sala_espera();
    pthread_cond_signal(&condc);
    pthread_mutex_unlock(&the_mutex);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads_clientes[N];
    pthread_t thread_barbero;
    int i;

    srand(clock()); // Semilla del generador aleatorio de numeros

    pthread_mutex_init(&the_mutex, 0);
    pthread_cond_init(&condp, 0);
    pthread_cond_init(&condc, 0);

    // Generamos los hilos de los N clientes
    for (i = 0; i < N; i++)
    {
        if (pthread_create(&threads_clientes[i], NULL, &cliente, (void *)(i+1)))
        { // Hubo error, aun asi no abortamos
            perror("Error en pthread_create()");
        }
        printf("Generado el cliente %d\n", i+1);
    }
    // Generamos un hilo para el barbero
    if (pthread_create(&thread_barbero, NULL, &barbero, (void *)NULL))
    { // Hubo error, aun asi no abortamos
        perror("Error en pthread_create()");
    }
    printf("Generado el barbero\n");

    // El hilo principal espera a que acaben los demas
    for (i = 0; i < N; i++)
    {
        if (pthread_join(threads_clientes[i], NULL))
        {
            perror("Error en pthread_join()");
        }
    }
    if (pthread_join(thread_barbero, NULL))
    {
        perror("Error en pthread_join()");
    }

    pthread_cond_destroy(&condp);
    pthread_cond_destroy(&condc);
    pthread_mutex_destroy(&the_mutex);

    exit(EXIT_SUCCESS);
}
