#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>

#define MAX_BUFFER 6        /* tamaño del buffer */
#define DATOS_A_PRODUCIR 50 /* número de datos a producir */
#define SLEEP_MAX_TIME 4    /* los sleeps serán de un máximo de 4 segundos */

mqd_t almacen1; /* cola de entrada de mensajes para el productor */
mqd_t almacen2; /* cola de entrada de mensajes para el consumidor */

// Devuelve el ítem asociado a un mensaje
int extraer_elemento(char *mensaje)
{
    return (int)*mensaje; // El mensaje es un sólo char que contiene un valor del 1 al 100, así que se devuelve el valor tal cual.
}

// Consume un item del buffer
void consumir(int numero)
{
    sleep(((int)rand()) % SLEEP_MAX_TIME); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
}

// Implementa la función de las diapositivas
void consumidor()
{
    int num_elementos_restantes;     // El numero de elementos que faltan por consumir
    char mensaje_elemento_producido; // Aquí recibiremos el mensaje
    int item;                        // El ítem que se nos pasa como mensaje

    for (num_elementos_restantes = MAX_BUFFER; num_elementos_restantes > 0; num_elementos_restantes--)
    { // Enviamos al productor tantos mensajes vacíos como huecos haya en el buffer
        if (mq_send(almacen1, NULL, 0, 0))
        { // Enviamos un mensaje vacío al productor, para indicarle que necesitamos que produzca un elemento. Usamos almacen1 y un mensaje vacío, por lo que la longitud (tercer argumento) es 0. El cuarto argumento, la prioridad, es 0 también.
            perror("Error en mq_send()");
        }
        printf("(C) He enviado un mensaje vacío al productor\n");
    }

    for (num_elementos_restantes = DATOS_A_PRODUCIR - MAX_BUFFER; num_elementos_restantes > 0; num_elementos_restantes--) // Tras enviar MAX_BUFFER mensajes vacíos, tenemos que seguir mandando mensajes y consumiendo elementos, hasta que lleguemos al máximo que queremos producir.
    {
        if (mq_receive(almacen2, &mensaje_elemento_producido, sizeof(mensaje_elemento_producido), NULL) == -1)
        { // Recibimos un mensaje del productor. Si no hay un mensaje disponible, la función se bloquea hasta que se recibe un mensaje. Lo recibimos en la cola almacen2, y lo guardamos en la dirección de mensaje_elemento_producido. El tamaño es el de un char, y el cuarto argumento es NULL porque no nos interesa saber cuál era la prioridad del mensaje (debería ser 0 en todos).
            perror("Error en mq_receive");
        }
        item = extraer_elemento(&mensaje_elemento_producido); // Obtenemos el ítem del mensaje
        if (mq_send(almacen1, NULL, 0, 0))
        { // Enviamos un mensaje vacío al productor. Esto le indica que queremos que nos envíe otro ítem más.
            perror("Error en mq_send()");
        }
        consumir(item); // Lo consumimos
        printf("(C) He consumido: %d\n", item);
    }

    for (num_elementos_restantes = MAX_BUFFER; num_elementos_restantes > 0; num_elementos_restantes--) // Finalmente, tenemos que consumir MAX_BUFFER mensajes, sin pedirle al productor que genere más.
    {
        if (mq_receive(almacen2, &mensaje_elemento_producido, sizeof(mensaje_elemento_producido), NULL) == -1)
        { // Recibimos un mensaje del productor. Si no hay un mensaje disponible, la función se bloquea hasta que se recibe un mensaje. Lo recibimos en la cola almacen2, y lo guardamos en la dirección de mensaje_elemento_producido. El tamaño es el de un char, y el cuarto argumento es NULL porque no nos interesa saber cuál era la prioridad del mensaje (debería ser 0 en todos).
            perror("Error en mq_receive");
        }
        item = extraer_elemento(&mensaje_elemento_producido); // Obtenemos el ítem del mensaje
        consumir(item);                                       // Lo consumimos
        printf("(C) He consumido: %d\n", item);
    }

    printf("(C) He acabado!\n");
}

int main(int argc, char **argv)
{
    struct mq_attr attr;            // Estructura que almacena parámetros sobre la cola de mensajes
    attr.mq_maxmsg = MAX_BUFFER;    // La cola tendrá un máximo de MAX_BUFFER mensajes.
    attr.mq_msgsize = sizeof(char); // El tamaño de mensajes para ambas colas será de un char como máximo. Realmente, el consumidor envía mensajes vacíos al productor, pero simplemente ignoraremos su contenido.

    srand(clock()); // Semilla del generador aleatorio de numeros

    /* Apertura de los buffers */
    almacen1 = mq_open("/ALMACEN1", O_WRONLY, 0777, &attr); // Abrimos la cola en modo sólo escritura, pero no la creamos (es responsabilidad del productor, que tiene que ser el primero en ejecutarse)
    almacen2 = mq_open("/ALMACEN2", O_RDONLY, 0777, &attr); // Abrimos la cola en modo sólo lectura, pero no la creamos
    if ((almacen1 == -1) || (almacen2 == -1))
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    consumidor();  // Código principal del consumidor

    mq_close(almacen1); // Cerramos las colas en este proceso
    mq_close(almacen2);

    // Borrado de las colas
    mq_unlink("/ALMACEN1");
    mq_unlink("/ALMACEN2");

    exit(EXIT_SUCCESS);
}