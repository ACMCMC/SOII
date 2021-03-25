# Aporte al foro

Partiendo del código que presenté para el problema de las sumas, lo he modificado para implementar la solución de Peterson, quedando el código que realiza el sumatorio escrito del siguiente modo:

void sum(int hilo)
{
int j;
for (j = hilo; j <= M; j += T)
{
entrar_region(hilo);
suma = suma + j;
salir_region(hilo);
}
}


He desarrollado también un script que realiza las mismas pruebas que yo, de forma automática, con sumatorios de 0 a 10 hasta 0 a 5000. Los datos se guardan en un fichero aparte para que puedan ser analizados posteriormente.

En mi caso, he obtenido los siguientes resultados (estadisticas.txt):



En la gráfica se muestra el error absoluto (y) sobre el límite superior del sumatorio (x). La curva de tendencia aparece destacada en rojo. Se puede apreciar que, en el caso de mi código, la solución de Peterson no proporciona exclusión mutua, ya que se producen carreras críticas. La magnitud del error tiende a aumentar con el volumen de las sumas, pero esto no es destacable para lo que nos importa, ya que simplemente por producirse carreras críticas, podemos concluir que la solución de Peterson no está funcionando adecuadamente.

Para entender el fenómeno, debemos entender que el procesador puede estar reordenando los accesos a memoria. Esto se describe de forma detallada en [1], pero intentaré resumirlo aquí:

Básicamente, la idea es que los procesadores x86-64 de Intel (el modelo de mi máquina), se aseguran de que las cargas se ejecutan por orden, al igual que las escrituras, entre otras garantías que nos ofrecen, pero puede darse el caso de que las cargas se adelanten a las escrituras. Es decir, si en pseudoMIPS (sólo lo uso para facilitar la comprensión, ya que en realidad mi máquina tiene el conjunto x86-64, como comentaba) tenemos:

addi $t0, $t0, 1       ; cargamos 1 en $t0
sw $t0, x              ; x valía 0, ahora pasa a valer 1
lw $t1, x              ; debería cargarse 1 en $t1

Podemos, al menos en principio, asumir que al final de la tercera instrucción, $t1 valdrá 1. Pues bien, esto no tiene por qué ser así. Y es que, como estamos empezando a ver en Arquitectura de Computadores, los procesadores pueden reordenar la ejecución de determinadas instrucciones para agilizar el procesamiento. En este caso, se puede ejecutar fuera de orden una instrucción de escritura con respecto a la de lectura (leemos el valor antes de hacer el sw). Por tanto, podría darse el caso de que $t1=0.

Cuando esto sucede en la ejecución del algoritmo de Peterson, puede romper la eficacia del mismo. Si nos fijamos en lo que se hace, entrar_region() está escrito así:

void entrar_region(int proceso)
{
    int otro;
    otro = 1 - proceso;
    interesado[proceso] = TRUE;
    turno = proceso;
    while (turno == proceso && interesado[otro] == TRUE) {};
}


Lo que tenemos es una escritura a interesado[0], o interesado[1], y a turno. Después, leemos la posición complementaria de interesado[], y también el valor de turno. Esto implica, representando los dos procesos, el siguiente flujo de ejecución (sin representar todas las instrucciones):

P0                                             P1
escribir(interesado[0], TRUE)                  escribir(interesado[1], TRUE)
escribir(turno, 0)                             escribir(turno, 1)
leer(turno)                                    leer(turno)
leer(interesado[1])                            leer(interesado[0])

Pero si tenemos en cuenta que las lecturas se pueden reordenar para hacerse antes de las escrituras, entonces puede darse el siguiente caso:

P0                                             P1
leer(turno)                                    leer(turno)
leer(interesado[1])                            leer(interesado[0])
escribir(interesado[0], TRUE)                  escribir(interesado[1], TRUE)
escribir(turno, 0)                             escribir(turno, 1)

Como se puede ver, esto implica que la condición del while se va a comprobar erróneamente. Independientemente del valor de turno, sabemos seguro que interesado[proceso]=FALSE antes de la escritura, así que el while() no va a funcionar.

Esta reordenación de las lecturas/escrituras no tiene por qué ocurrir necesariamente, pero sí es cierto que no tenemos la garantía de que no suceda. Y de esta forma podemos entender las carreras críticas que se siguen dando.

Finalmente, aparte de lo ya comentado, podemos tener en cuenta lo que ocurre al compilar con -O3, en vez de -O0, que es la opción que estaba usando hasta ahora. Con -O0, el código se compila así (para el ejemplo, puede suponerse que rax "es más o menos lo mismo" que eax):

entrar_region:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-20], edi
        mov     eax, 1                                     ; colocamos un 1 en eax
        sub     eax, DWORD PTR [rbp-20]                    ; eax = eax - proceso (0 o 1)
        mov     DWORD PTR [rbp-4], eax                     ; otro = eax
        mov     eax, DWORD PTR [rbp-20]                    ; eax = proceso
        cdqe
        mov     DWORD PTR interesado[0+rax*4], 1           ; interesado[proceso] = 1
        mov     eax, DWORD PTR [rbp-20]                    ; eax = proceso
        mov     DWORD PTR turno[rip], eax                  ; turno = eax
        nop
.L3:
        mov     eax, DWORD PTR turno[rip]                  ; eax = turno
        cmp     DWORD PTR [rbp-20], eax                    ; eax==proceso?
        jne     .L4                                        ; si no, saltamos a .L4 (salir de la función)
        mov     eax, DWORD PTR [rbp-4]                     ; eax = otro
        cdqe
        mov     eax, DWORD PTR interesado[0+rax*4]         ; eax = interesado[otro]
        cmp     eax, 1                                     ; eax==1?
        je      .L3                                        ; si sí, volvemos a comprobar las condiciones, si no, salimos de la función
.L4:
        nop
        pop     rbp
        ret                                                ; salimos de la función

Con -O3, en cambio, se compila así:

entrar_region:
        movsx   rax, edi                                   ; colocamos en rax el número del proceso (0 o 1), que estaba en edi
        mov     DWORD PTR turno[rip], edi                  ; escribimos en turno el número
        mov     DWORD PTR interesado[0+rax*4], 1           ; escribimos en interesado[proceso] un 1
        mov     eax, 1                                     ; escribimos 1 en eax
        sub     eax, edi                                   ; eax = 1 - eax (eax = otro)
        cdqe
        cmp     DWORD PTR interesado[0+rax*4], 1           ; interesado[otro]==1?
        jne     .L8                                        ; si no, saltamos a .L8
.L10:
        jmp     .L10                                       ; bucle infinito
.L8:
        ret                                                ; salimos de la función

Sin analizar todas las instrucciones, podemos observar algo a simple vista: -O3 abrevia mucho la longitud de la rutina. Por tanto, ya podemos deducir que algo se está saltando. Más concretamente, se pueden ver las siguientes diferencias:

Con O0, el orden de las instrucciones es el mismo que en el código de C. En cambio, con O3, sí se reordenan las instrucciones:

1. turno = proceso;
2. interesado[proceso] = TRUE;
3. otro = 1 - proceso;

Pese a todo, esto no nos plantearía problemas. La cuestión es que, con O3, si (turno == proceso && interesado[otro] == TRUE) se cumple, entramos en un bucle infinito. Esto es porque gcc deduce que, si acabamos de escribir que el turno es nuestro, entonces esa variable seguirá valiendo lo mismo (no imagina que hay una carrera crítica), así que directamente no comprueba esa condición. Además, si la primera vez que la evaluamos, se cumple que interesado[otro]==TRUE, como no lo modificamos desde nuestro proceso, supone que va a seguir siendo así para siempre, y entramos en el bucle infinito.

Esto lo que implica es que, directamente, el código compilado con -O3, en general podrá quedarse en un bucle infinito.

Después, independientemente de que compilemos con -O3 o con -O0, el problema de la reordenación de lecturas/escrituras es el mismo que describía antes, por lo que seguiremos teniendo carreras críticas en ambos casos. En todo caso, es destacable que en -O3, excepto en el caso de interesado[otro], nunca se llegan a leer las variables de memoria (se trabaja con todos los datos directamente a partir de los registros). Lo único que se realizan son las escrituras, ya que el compilador no imagina que otro proceso podría estar escribiendo en nuestra misma región de memoria.

En definitiva, lo que podemos comprobar es que la solución de Peterson, en los procesadores modernos, no soluciona nada. Incluso compilando sin optimizaciones, las propias optimizaciones que hacen los procesadores de por sí hacen que se invalide su algoritmo, que depende directamente de que las instrucciones de lectura y escritura se realicen en orden.

Finalmente, adjunto una imagen de las características de mi sistema:



Los archivos asociados los adjunto a esta entrada. Saludos!

[1] https://bartoszmilewski.com/2008/11/05/who-ordered-memory-fences-on-an-x86/