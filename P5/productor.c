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

// Se encarga de construir un mensaje, pasándole un puntero a cadena de caracteres y un ítem
void construir_mensaje(char *mensaje, int item)
{
    *mensaje = (char)item; // El mensaje es un sólo char que contiene un valor del 1 al 100, así que hacemos que el lugar donde apunta mensaje sea el valor de item.
}

// Produce un item, para introducirlo en el buffer
int producir()
{
    sleep(((int)rand()) % SLEEP_MAX_TIME); // Introducimos una espera aleatoria entre 0 y SLEEP_MAX_TIME
    return ((int)rand() % 100) + 1;        // Numeros entre 1 y 100, para que quepan en un char (es decir, tomamos el valor del char como el valor del ítem, e ignoramos su representación ASCII. Por ejemplo, el 65 se interpretará como ese número, no como la letra 'A'). Si no, podríamos enviar varios chars.
}

// Implementa la funcion de las diapositivas
void productor()
{
    int num_elementos_restantes;     // El numero de elementos que faltan por producir
    char mensaje_vacio_consumidor;   // El mensaje con el elemento producido, que le mandamos al consumidor
    char mensaje_elemento_producido; // El mensaje con el elemento producido, que le mandamos al consumidor
    int item;                        // El ítem que le pasamos al consumidor

    for (num_elementos_restantes = DATOS_A_PRODUCIR; num_elementos_restantes > 0; num_elementos_restantes--) // Vamos a iterar mientras aún queden ítems por producir
    {
        if (mq_receive(almacen1, &mensaje_vacio_consumidor, sizeof(mensaje_vacio_consumidor), NULL))
        { // Recibimos un mensaje del consumidor. Si no hay un mensaje disponible, la función se bloquea hasta que se recibe un mensaje. Lo recibimos en la cola almacen1, y como no nos interesa guardarlo (es un mensaje vacío), el puntero al lugar donde guardarlo es NULL, la longitud del buffer al que apunta (NULL) es 0, y tampoco nos interesa saber la prioridad del mensaje (cuarto argumento, que es NULL también). Se supone que la prioridad siempre es la misma (0).
            perror("Error en mq_receive");
        }
        printf("(C) He recibido un mensaje vacío del consumidor\n");
        item = producir(); // Obtenemos el ítem del mensaje
        printf("(P) He producido: %d\n", item);
        construir_mensaje(&mensaje_elemento_producido, item);
        if (mq_send(almacen2, &mensaje_elemento_producido, sizeof(char), 0))
        { // Enviamos un mensaje con el ítem al consumidor. Lo hacemos a través de almacen2, enviando mensaje_elemento_producido, con un tamaño de un char, y prioridad 0.
            perror("Error en mq_send()");
        }
    }

    printf("(P) He acabado!\n");
}

int main(int argc, char **argv)
{
    struct mq_attr attr;            // Estructura que almacena parámetros sobre la cola de mensajes
    attr.mq_maxmsg = MAX_BUFFER;    // La cola tendrá un máximo de MAX_BUFFER mensajes.
    attr.mq_msgsize = sizeof(char); // El tamaño de mensajes para ambas colas será de un char como máximo. Realmente, el consumidor envía mensajes vacíos al productor, pero simplemente ignoraremos su contenido.

    srand(clock()); // Semilla del generador aleatorio de numeros

    /* Borrado de los buffers de entrada por si existían de una ejecución previa*/
    mq_unlink("/ALMACEN1");
    mq_unlink("/ALMACEN2");

    /* Apertura de los buffers */
    almacen1 = mq_open("/ALMACEN1", O_CREAT | O_RDONLY, 0777, &attr); // Creamos la cola de mensajes almacen1, nuestro acceso a ella será en modo sólo lectura
    almacen2 = mq_open("/ALMACEN2", O_CREAT | O_WRONLY, 0777, &attr); // Creamos la cola de mensajes almacen2, nuestro acceso a ella será en modo sólo escritura
    if ((almacen1 == -1) || (almacen2 == -1))
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    productor();

    mq_close(almacen1); // Cerramos las colas en este proceso
    mq_close(almacen2);

    exit(EXIT_SUCCESS);
}