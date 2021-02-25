# Aporte al foro

Haz un programa en el que dos procesos accedan a ficheros para leer y escirbir similar al que vimos en clase.

Comprueba los resultados obtenidos. Quizás sea interesante probar con ficheros de diferentes tamaños. Ejecútalo varias veces por si el comportamiento cambia.

Comenta lo observado.

Temas adicionales:

1- Hacer un estudio cuantitativo de la relación entre la probabilidad de que ocurran carreras críticas y el tamaño del fichero.

2- Hacer un estudio cuantitativo de la relación entre la probabilidad de que ocurran carreras críticas y el número de procesos involucrados.

---

Antes de empezar, convendría aclarar que por "probabilidad de que ocurran carreras críticas" entiendo en este estudio "probabilidad de que al menos un carácter no se corresponda con el original". Esto es distinto del número de errores del archivo (que es acumulativo).

### 1- Hacer un estudio cuantitativo de la relación entre la probabilidad de que ocurran carreras críticas y el tamaño del fichero.

Mis compañeros han tratado este tema en detalle, así que sería bastante redundante entrar a explicar de nuevo este fenómeno. Me limitaré a decir que tenemos una probabilidad X de que un carácter sea escrito de forma inconsistente con el original, independiente del número de caracteres total del archivo.

Es decir, que a mayor tamaño del archivo, mayor número de incoherencias en el resultado, pero porque por cada carácter tenemos una probabilidad constante de tener un error, y estos errores se van sumando. El porcentaje de errores es más o menos constante, en mi caso un 85% (se puede ver en la gráfica de abajo).

### 2- Hacer un estudio cuantitativo de la relación entre la probabilidad de que ocurran carreras críticas y el número de procesos involucrados.

Para estudiar este fenómeno, he desarrollado un programa en C que ejecuta pruebas con distinto número de procesos (estadisticas.c). El código fuente está disponible abajo. El programa realiza pruebas con distintos tamaños de archivo (parte de un tamaño inicial, que va incrementando hasta alcanzar un tamaño final), y distinto número de procesos (que también va aumentando). Luego guarda las estadísticas del número de errores para cada par (tamaño de archivo, número de procesos), de forma que se puedan tratar en un programa de análisis estadístico. También fue necesario adaptar el código del Campus Virtual, para que ejecute un número arbitrario de procesos en paralelo (se le pasan como parámetro por línea de comandos).

Los resultados que aporta el programa se pueden ver en la siguente gráfica:

\[Gráfica\]

El plano que interpola los puntos se corresponde con la fórmula:

```
z = 0,85*x + 11*y - 20, donde:

z = número de errores en el archivo

x = tamaño del archivo

y = número de procesos involucrados
```

Podemos ver que, a mayor tamaño del archivo, mayor número de errores (conclusión del punto 1). Este es el eje de mayor longitud, ya que las pruebas se han realizado desde 100 hasta 1000 caracteres, con incrementos de 100.

El eje de menor longitud es el número de procesos. El número de procesos es 2, 3, 4, y 5. No ejecuto las pruebas con un solo proceso porque no tiene sentido (no se producirían carreras críticas).

Analicemos un poco más la gráfica para entender mejor qué nos dice:

\[Imagen de la gráfica\]

Las líneas indican los conjuntos de nivel (que estudiamos en Fundamentos de Matemáticas). Para una misma línea, hay el mismo número de errores (el número de errores es el eje vertical). Lo que tenemos es que si nos situamos en un punto de una línea (por ejemplo, el punto rojo que he dibujado más a la derecha), todos los demás puntos que estén en esa línea tendrán el mismo número de errores. Y si nos movemos hacia el punto rojo de más arriba, a la izquierda, lo que tendremos es el mismo número de errores, pero (ojo), con menor tamaño de archivo pero más procesos ejecutándose a la vez. Esto demuestra la **relación directa que existe entre número de errores, tamaño del archivo, y número de procesos involucrados**.

Ahora que entendemos mejor la gráfica, conviene justificar mi decisión de elegir un rango en el número de procesos involucrados tan escueto. Solo llego hasta 5 procesos en mi gráfica, porque mi procesador tiene 4 núcleos, y si elijo números más grandes, el sistema simplemente se colapsa y el porcentaje de errores asciende prácticamente al 100%. Hice las pruebas con hasta 500 procesos hijo, y lo que conseguía eran gráficas planas en el eje del número de procesos (esta gráfica, en cambio, tiene una pendiente del 11% en el eje del número de procesos). Si tenemos tantos procesos, lo que sucederá es que no todos se ejecutarán a la vez. La CPU tendrá que alternar entre los hijos (500, siguiendo con el ejemplo), dependiendo de la política del planificador.

Podríamos tener una situación, por ejemplo, en la que un proceso obtiene su quantum de ejecución, pero lo usa para realizar un fscanf (que implica una llamada al sistema), y pasar al estado dormido mientras el sistema resuelve la llamada que hizo. Será entonces necesario un cambio de contexto para que otro proceso se ejecute, y éste vuelva a quedar dormido rápidamente también. Y cuando se resuelvan las llamadas al sistema y los procesos se reanuden, podrán haber ocurrido por el medio muchas otras llamadas al sistema de otros procesos distintos, incluso paralelamente desde otros núcleos de la CPU, y al final lo que tendremos es un sistema colapsado por la conmutación entre todos estos procesos y la resolución de sus llamadas al sistema, haciendo casi imposible que se escriba un solo carácter de forma correcta (un sistema saturado, como decía, que suba la tasa de errores por carácter al 100% de forma que no se aprecie la relación entre número de errores y número de procesos involucrados).

Volviendo al caso de la gráfica que nos ocupa, lo que podemos ver, en definitiva, es que sí se producen más errores a mayor número de procesos, y esto es lógico: si tenemos 2 procesos el sistema aún no está colapsado, y así para números hasta superar considerablemente la capacidad del procesador. También es importante comentar que es lógico que el número de procesos involucrados no tenga una influencia tan marcada sobre el número total de errores del archivo de salida: al fin y al cabo, ya que con dos procesos se produce una tasa de errores importante, no hay mucho margen a que ésta pueda crecer añadiendo más procesos que lean y escriban simultáneamente, por lo que es lógico un aumento del número de errores, pero no demasiado marcado (recordemos que en mi ordenador tiene una pendiente del 11%). También conviene señalar que se aprecian algunos valores desviados, pero no creo que sean relevantes para el estudio, y por tanto he decidido omitir mencionarlos.

En conclusión, hay una relación directa entre número de procesos involucrados, y probabilidad de que ocurran carreras críticas, y a mayor tamaño de fichero, mayor número de errores (aunque la probabilidad por carácter de que ocurran carreras críticas es la misma).