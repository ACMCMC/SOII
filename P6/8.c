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

#define TIEMPO_CRUZAR_PUENTE 2
#define TIEMPO_PASEAR 30
#define NUM_COCHES 8

#define IZQUIERDA 0
#define DERECHA 1

// Codigos de color para formatear la salida en consola
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/*================================================================================
                                          ___               
                                         (   )              
   .-..    ___  ___    .--.    ___ .-.    | |_       .--.   
  /    \  (   )(   )  /    \  (   )   \  (   __)    /    \  
 ' .-,  ;  | |  | |  |  .-. ;  |  .-. .   | |      |  .-. ; 
 | |  . |  | |  | |  |  | | |  | |  | |   | | ___  |  | | | 
 | |  | |  | |  | |  |  |/  |  | |  | |   | |(   ) |  |/  | 
 | |  | |  | |  | |  |  ' _.'  | |  | |   | | | |  |  ' _.' 
 | |  ' |  | |  ; '  |  .'.-.  | |  | |   | ' | |  |  .'.-. 
 | `-'  '  ' `-'  /  '  `-' /  | |  | |   ' `-' ;  '  `-' / 
 | \__.'    '.__.'    `.__.'  (___)(___)   `.__.    `.__.'  
 | |                                                        
(___)                                                       
                                                                             

Aldan Creo Marino, SOII 2020/21

(no uso tildes por si hubiera problemas de formato)

Comentario al codigo:
    Este problema tiene una cierta complejidad anadida, por el hecho de que se
    pide que crucen todos los coches que estan en un mismo lado conjuntamente.
    
    Lo que he hecho para resolverlo ha sido basarme en la resolucion del problema
    de los lectores y los escritores. Aqui, uso 4 mutexes para regular el
    acceso al puente:
    -   mutex_cuenta_izquierda: Su proposito es regular el acceso a la variable
        cuenta_izquierda, para que no se produzcan carreras criticas.
    -   mutex_cuenta_derecha: Tiene la misma funcion.
    -   puente: regula el acceso al recurso del puente en un sentido. Es decir,
        todos los coches que crucen en un sentido comparten el acceso al puente,
        y lo usaran para cruzar cuando ningun otro coche este cruzando.
    -   cruzando: La uso para evitar que, dentro del conjunto de coches que
        quieren cruzar desde el mismo sentido, cruzen todos a la vez. Es decir,
        un coche que quiera cruzar debera adquirir este mutex.
    
    Por tanto, tengo una especie de estructura de acceso exclusivo en dos fases.
    Por un lado, el primer coche en cruzar bloqueara el uso del puente para su
    lado del puente. Por ejemplo, los coches que crucen desde la izquierda.
    
    A continuacion, una vez que sabemos que no hay coches que intenten cruzar
    desde el otro sentido (ya que tenemos el uso del puente reservado para
    nuestro lado), entonces cada coche adquirira "cruzando", durante 2 segundos,
    y lo liberara cuando acabe de cruzar.

    Es decir, en mi codigo, es como si tuviera dos tipos de lectores, que son
    incompatibles entre si, pero con la diferencia de que en el problema original
    de los lectores-escritores, todos los lectores podian leer a la vez, y en mi
    codigo, solo uno de ellos puede usar el recurso (cruzar el puente) a la vez.
================================================================================*/

int *cuenta_izquierda; // Lleva la cuenta del numero de coches que hay en el lado izquierdo del puente
int *cuenta_derecha;   // Lleva la cuenta del numero de coches que hay en el lado derecho del puente

sem_t *mutex_cuenta_izquierda, *mutex_cuenta_derecha, *puente, *cruzando, *num_procesos; // Variables semaforo

// Espera entre 1 y 30 segundos
void pasear()
{
    sleep(((int)rand()) % (TIEMPO_PASEAR - 1) + 1); // Introducimos una espera aleatoria
}

// Funcion que hace que un coche cruce el puente
void cruzar_puente()
{
    sem_wait(cruzando);          // Usamos el mutex que impide que otro coche cruce a la vez. Todos los coches del mismo sentido que quieran cruzar estaran bloqueados esperando aqui.
    sleep(TIEMPO_CRUZAR_PUENTE); // Esperamos 2 segundos
    sem_post(cruzando);          // Permitimos que pase el siguiente coche
}

// Devuelve IZQUIERDA o DERECHA aleatoriamente
int escoger_sentido()
{
    int sentido;
    int num_aleatorio;
    num_aleatorio = ((int)rand() % 2);                  // 0 o 1
    sentido = num_aleatorio == 0 ? IZQUIERDA : DERECHA; // Dependiendo del resultado, elegimos el sentido
    return sentido;
}

// Funcion que ejecutan los coches que cruzan desde la izquierda. La idea es la misma que abajo, asi que me remito a los comentarios que hay en cruzar_derecha().
void cruzar_izquierda(int id)
{
    sem_wait(mutex_cuenta_izquierda);
    *cuenta_izquierda = *cuenta_izquierda + 1;
    if (*cuenta_izquierda == 1)
    {
        sem_wait(puente);
    }
    sem_post(mutex_cuenta_izquierda);
    cruzar_puente();
    sem_wait(mutex_cuenta_izquierda);
    *cuenta_izquierda = *cuenta_izquierda - 1;
    if (*cuenta_izquierda == 0)
    {
        sem_post(puente);
    }
    sem_post(mutex_cuenta_izquierda);
}

// Funcion que ejecutan los coches que cruzan desde la derecha
void cruzar_derecha(int id)
{
    sem_wait(mutex_cuenta_derecha);        // Mutex para el acceso a la variable cuenta_derecha
    *cuenta_derecha = *cuenta_derecha + 1; // Incrementamos la cuenta de coches esperando de este lado
    if (*cuenta_derecha == 1)              // Si este es el primer coche en cruzar desde la derecha, bloqueamos el uso del puente para los coches que cruzan desde la derecha.
    {
        sem_wait(puente); // Si el puente esta en uso desde el otro lado, nos quedamos bloqueados aqui. El resto de coches que quieran cruzar desde nuestro lado tambien se quedaran bloqueados, pero por el mutex en cuenta_derecha.
    }
    sem_post(mutex_cuenta_derecha);        // Liberamos el mutex para la variable cuenta_derecha
    cruzar_puente();                       // El coche cruza el puente. Dentro de la funcion se usa exclusion mutua (con "cruzando") para evitar que dos coches crucen a la vez en el mismo sentido.
    sem_wait(mutex_cuenta_derecha);        // Bloqueamos el mutex para el acceso a la variable cuenta_derecha
    *cuenta_derecha = *cuenta_derecha - 1; // Ahora hay un coche menos para cruzar desde este lado
    if (*cuenta_derecha == 0)              // Si este es el ultimo coche que faltaba por cruzar (podemos confiar en que el valor de cuenta_derecha es correcto, porque no puede haber carreras criticas sobre la variable, ya que estamos dentro de una zona de exclusion mutua)
    {
        sem_post(puente); // Liberamos el mutex del puente. Ahora, si hay coches que quieran cruzar desde la izquierda, se desbloquearan y podran adquirir el mutex del puente ellos.
    }
    sem_post(mutex_cuenta_derecha); // Liberamos el mutex para la variable cuenta_derecha
}

// Todos los coches ejecutan esta funcion. Enlaza con cruzar_izquierda(), o cruzar_derecha()
void coche(int id)
{
    int sentido;

    pasear();                    // Empezamos paseando de 1 a 30 s
    sentido = escoger_sentido(); // Se elige aleatoriamente
    if (sentido == IZQUIERDA)
    {
        printf(ANSI_COLOR_BLUE "[%d] Voy a cruzar el puente desde la IZQUIERDA\n" ANSI_COLOR_RESET, id);
        cruzar_izquierda(id); // Cruzamos desde la izquierda
        printf(ANSI_COLOR_BLUE "[%d] He cruzado el puente desde la IZQUIERDA\n" ANSI_COLOR_RESET, id);
    }
    else
    {
        printf(ANSI_COLOR_RED "[%d] Voy a cruzar el puente desde la DERECHA\n" ANSI_COLOR_RESET, id);
        cruzar_derecha(id); // Cruzamos tambien, pero desde la derecha
        printf(ANSI_COLOR_RED "[%d] He cruzado el puente desde la DERECHA\n" ANSI_COLOR_RESET, id);
    }
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

    // Opcion de reiniciar
    if (argc == 2 && (strncmp("-r", argv[1], 2) == 0))
    {
        if (sem_unlink("/mutex_cuenta_izquierda"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/mutex_cuenta_derecha"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/puente"))
        {
            perror("Error en sem_unlink()");
            exit(EXIT_FAILURE);
        }
        if (sem_unlink("/cruzando"))
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

    // Probamos a abrir uno de los semaforos
    puente = sem_open("/puente", 0);
    // Si la funcion falla, es porque los semaforos no estan creados, asi que los creamos. O_CREAT no tiene efecto si el semaforo ya estaba creado.
    if (puente == SEM_FAILED)
    {
        printf("Semaforos creados.\n");
        puente = sem_open("/puente", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); // Usamos O_CREAT | O_EXCL porque en un caso extremo podria haber dos procesos que detectaran que los semaforos no existen al mismo tiempo. De esta forma, solo uno de ellos los creara y el otro saldra con codigo de error.
        num_procesos = sem_open("/num_procesos", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
        mutex_cuenta_izquierda = sem_open("/mutex_cuenta_izquierda", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        mutex_cuenta_derecha = sem_open("/mutex_cuenta_derecha", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        cruzando = sem_open("/cruzando", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    }
    else
    {
        // Abrimos el resto de semaforos normalmente, ya que estaban creados de antes
        num_procesos = sem_open("/num_procesos", 0);
        mutex_cuenta_izquierda = sem_open("/mutex_cuenta_izquierda", 0);
        mutex_cuenta_derecha = sem_open("/mutex_cuenta_derecha", 0);
        cruzando = sem_open("/cruzando", 0);
    }

    if (mutex_cuenta_derecha == SEM_FAILED || mutex_cuenta_izquierda == SEM_FAILED || cruzando == SEM_FAILED || puente == SEM_FAILED || num_procesos == SEM_FAILED)
    {
        perror("Error en sem_open()");
        exit(EXIT_FAILURE);
    }

    // Mapeamos la memoria compartida para los procesos. Ahora no hace falta un objeto anonimo compartido en memoria, ya que los hijos heredaran este mapeo que hacemos localmente.
    cuenta_izquierda = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    cuenta_derecha = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (cuenta_derecha == MAP_FAILED || cuenta_izquierda == MAP_FAILED)
    {
        perror("Error en mmap()");
        exit(EXIT_FAILURE);
    }

    if (sem_post(num_procesos)) // Incrementamos el semaforo que lleva la cuenta del numero de procesos
    {
        perror("Error en sem_post");
    }

    *cuenta_izquierda = 0; // Inicializamos la variable de cuenta a 0. Deberia venir ya inicializada, pero creo que es buena practica asegurarnos.
    *cuenta_derecha = 0;   // Lo mismo

    for ((i = 0, pid = -1); i < NUM_COCHES && pid != 0; i++)
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
            srand(clock()); // Reiniciamos la semilla del generador aleatorio de numeros
            coche(i);       // Ejecutamos el codigo
        }
        else
        {
            printf("Generado el coche %d\n", i);
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

    if (munmap(cuenta_izquierda, sizeof(int))) // Deshacemos los mapeos de memoria. Esto lo pueden hacer todos los procesos.
    {
        perror("Error en munmap()");
    }
    if (munmap(cuenta_derecha, sizeof(int)))
    {
        perror("Error en munmap()");
    }

    if (sem_close(mutex_cuenta_derecha))
    {
        perror("Error en sem_close");
    }
    if (sem_close(mutex_cuenta_izquierda))
    {
        perror("Error en sem_close");
    }
    if (sem_close(puente))
    {
        perror("Error en sem_close");
    }
    if (sem_close(cruzando))
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
        if (sem_unlink("/mutex_cuenta_derecha"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/mutex_cuenta_izquierda"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/puente"))
        {
            perror("Error en sem_unlink()");
        }
        if (sem_unlink("/cruzando"))
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
