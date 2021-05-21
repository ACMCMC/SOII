#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#define N 8                 // Numero de clientes
#define NUM_SILLAS_ESPERA 3 // El numero de sillas en la sala de espera
#define TIEMPO_ATENCION 4   // El tiempo que se tarda en atender a un cliente
#define TIEMPO_PASEO 30     // El tiempo que estan paseando los clientes como maximo

/*==========================================================================

 ___                            ___                                          
(   )                          (   )                           .-.           
 | |.-.     .---.   ___ .-.     | |.-.     .--.    ___ .-.    ( __)   .---.  
 | /   \   / .-, \ (   )   \    | /   \   /    \  (   )   \   (''")  / .-, \ 
 |  .-. | (__) ; |  | ' .-. ;   |  .-. | |  .-. ;  | ' .-. ;   | |  (__) ; | 
 | |  | |   .'`  |  |  / (___)  | |  | | |  | | |  |  / (___)  | |    .'`  | 
 | |  | |  / .'| |  | |         | |  | | |  |/  |  | |         | |   / .'| | 
 | |  | | | /  | |  | |         | |  | | |  ' _.'  | |         | |  | /  | | 
 | '  | | ; |  ; |  | |         | '  | | |  .'.-.  | |         | |  ; |  ; | 
 ' `-' ;  ' `-'  |  | |         ' `-' ;  '  `-' /  | |         | |  ' `-'  | 
  `.__.   `.__.'_. (___)         `.__.    `.__.'  (___)       (___) `.__.'_. 
                                                                             
                                                                             
Aldan Creo Marino, SOII 2020/21

Comentario al codigo:

    El codigo de este ejercicio es esencialmente el mismo que el de la
    practica del productor-consumidor con hilos, usando mutexes y variables
    de condicion.

    En este caso, el recurso por el que se compite son las sillas de la
    sala de espera. Un cliente se sienta una unica vez durante todo el
    programa, en una de las sillas de la sala de espera. Esto es lo mismo
    que tener un productor que genera un unico elemento en todo el programa.

    Si consigue una silla de la sala de espera, el cliente quedara a la
    espera de que un barbero le atienda. Es decir, una vez que consigue
    un lugar en el recurso compartido de memoria, espera a que el consumidor
    lo retire del mismo. Como se puede ver, la implementacion de este codigo
    es muy similar al que ya se hizo en practicas anteriores, ya que la idea
    del problema es esencialmente la misma.

    La unica diferencia reseñable seria que, en el caso de que la sala de
    espera este llena, el cliente no se duerme a la espera de que haya un
    hueco, como si ocurria con los productores de la otra practica, sino que
    siguen paseando durante un tiempo aleatorio y vuelven a intentar sentarse
    (espera activa).

    Como observacion adicional, cabe comentar que no hago comprobaciones de
    error en cada una de las llamadas al sistema. Esto es porque considero
    que hacerlas saturaria el codigo, y lo que aqui es mas relevante es que
    se pueda leer y entender facilmente. Si pusiera un if por cada llamada
    al sistema, quedaria un "codigo espagueti".
===========================================================================*/

pthread_mutex_t mutex_sala_espera;
pthread_cond_t condc;

int sillas[N];    // Las sillas de la sala de espera. Se representa como un buffer compartido de memoria
int cuentaSillas; // El numero de sillas ocupadas
int primeraSilla; // El lugar donde esta la primera silla ocupada

// Imprime por pantalla una representacion grafica de las sillas de la sala de espera
void imprimir_sala_espera()
{
    int i;
    printf("\t");
    for (i = 0; i < NUM_SILLAS_ESPERA; i++)
    {
        printf("--------");
    }
    printf("\n\t");
    for (i = 0; i < NUM_SILLAS_ESPERA; i++)
    {
        if (sillas[i] != 0)
        {
            printf("|(%-2d)|  ", sillas[i]);
        }
        else
        {
            printf("|(  )|  ");
        }
    }
    printf("\n\t");
    for (i = 0; i < NUM_SILLAS_ESPERA; i++)
    {
        printf("--------");
    }
    printf("\n\n");
}

// Espera entre 1 y 30 segundos
void pasear()
{
    sleep(((int)rand()) % (TIEMPO_PASEO - 1) + 1); // Introducimos una espera aleatoria
}

// Espera 4 segundos
void atenderCliente()
{
    sleep(TIEMPO_ATENCION); // Introducimos una espera aleatoria
}

// "Si entra un cliente en la barberia y todas las sillas estan ocupadas, abandona la barberia, y lo vuelve a intentar pasado un tiempo aleatorio"
void volverAIntentarTrasTiempoAleatorio()
{
    pasear();
}

// Indica si hay sillas libres en la sala de espera
int hay_sillas_libres_sala_espera()
{
    return cuentaSillas < NUM_SILLAS_ESPERA;
}

// Sienta a un cliente en la primera silla libre de la sala de espera
void sentarse_en_silla_espera(int idCliente)
{
    sillas[(primeraSilla + cuentaSillas) % NUM_SILLAS_ESPERA] = idCliente;
    cuentaSillas++;
}

// Levanta a un cliente de su silla en la sala de espera, devuelve su ID
int levantarse_de_silla_espera()
{
    int idCliente = sillas[primeraSilla];
    sillas[primeraSilla] = 0;
    primeraSilla = (primeraSilla + 1) % NUM_SILLAS_ESPERA;
    cuentaSillas--;
    return idCliente;
}

void *barbero(void *p)
{
    int num_clientes_restantes;
    int cliente;

    for (num_clientes_restantes = N; num_clientes_restantes > 0; num_clientes_restantes--)
    {
        pthread_mutex_lock(&mutex_sala_espera); // Adquirimos el acceso exclusivo a la sala de espera
        printf("(Barbero) Accedo a la sala de espera\n");
        while (cuentaSillas == 0)
        {
            printf("(Barbero) No hay nadie. Me voy a dormir\n");
            pthread_cond_wait(&condc, &mutex_sala_espera);
        }
        cliente = levantarse_de_silla_espera();
        imprimir_sala_espera();
        pthread_mutex_unlock(&mutex_sala_espera); // Liberamos el acceso a la sala de espera
        printf("(Barbero) Salgo de la sala de espera\n");
        printf("(Barbero) Atiendo al cliente %d\n", cliente);
        atenderCliente();
    }
    printf("(Barbero) He acabado mi trabajo!\n");
    return NULL;
}

void *cliente(void *p_num_cliente)
{
    int num_cliente = ((int *)p_num_cliente);
    int sentado = 0;

    do
    {
        pasear();
        pthread_mutex_lock(&mutex_sala_espera); // El acceso a la sala de espera es solo nuestro
        printf("(Cliente %d) Accedo a la sala de espera\n", num_cliente);
        if (hay_sillas_libres_sala_espera())
        {
            sentarse_en_silla_espera(num_cliente);
            printf("(Cliente %d) Estoy esperando en la sala de espera\n", num_cliente);
            sentado = 1;
        }
        else
        {
            printf("(Cliente %d) La sala esta llena. Vuelvo a pasear\n", num_cliente);
        }
        imprimir_sala_espera();
        pthread_cond_signal(&condc); // Despertamos al barbero si esta dormido
        pthread_mutex_unlock(&mutex_sala_espera); // Liberamos el acceso a la sala de espera
    } while (!sentado);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads_clientes[N];
    pthread_t thread_barbero;
    int i;

    srand(clock()); // Semilla del generador aleatorio de numeros

    pthread_mutex_init(&mutex_sala_espera, 0);
    pthread_cond_init(&condc, 0);

    // Generamos los hilos de los N clientes
    for (i = 0; i < N; i++)
    {
        if (pthread_create(&threads_clientes[i], NULL, &cliente, (void *)(i + 1)))
        { // Hubo error, aun asi no abortamos
            perror("Error en pthread_create()");
        }
        printf("Generado el cliente %d\n", i + 1);
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

    pthread_cond_destroy(&condc);
    pthread_mutex_destroy(&mutex_sala_espera);

    exit(EXIT_SUCCESS);
}
