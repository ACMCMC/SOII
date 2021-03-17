#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
if [ $# -eq 0 ] || [ ! -f "$1" ] ; then # Si no se han proporcionado argumentos...
echo "No se ha definido la ruta del archivo o no existe. Uso: $0 [ruta]"
else # Si se han proporcionado, seguimos normalmente
grep -Po '^(?:\d{1,3}.){3}\d{1,3}' $1 | sort | uniq -c > /tmp/access_ip.log # Extraer las direcciones IP a un fichero
grep -Po '\S+\s\S+"\s404\s\d+$' $1 | grep -Po '^\S+' > /tmp/access_404.log # Extraer únicamente las URLs con respuesta del servidor 404
num_ips=`wc -l /tmp/access_ip.log | awk 'END {print $1}'` # Contamos el número de líneas que hay en el fichero de IPs, ése es el número que nos piden. Como wc -l saca el número seguido del nombre del archivo, pasamos esa salida a awk y nos quedamos con la primera columna (el número)
num_404=`wc -l /tmp/access_404.log | awk 'END {print $1}'` # Contamos el número de líneas también en el fichero de respuestas con 404. También es lo que nos piden. Como wc -l saca el número seguido del nombre del archivo, pasamos esa salida a awk y nos quedamos con la primera columna (el número)
num_accesos=`wc -l $1 | awk 'END {print $1}'` # El número total de accesos es el número de líneas en el fichero original. Como wc -l saca el número seguido del nombre del archivo, pasamos esa salida a awk y nos quedamos con la primera columna (el número)
echo "$num_ips direcciones diferentes extraídas en /tmp/access_ip.log"
echo "$num_404 accesos a URLs no encontradas extraídos en /tmp/access_404.log"
echo "$num_accesos accesos en total:"
# Esta parte podría hacerse fácilmente con un bucle for, pero se nos indicó en clase que no debíamos usarlo
mes='Dec' # Ponemos el mes como una variable, no es estrictamente necesario
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'` # El número de días en Enero. Se puede obtener fácilmente contando con awk el número de campos totales
accesos_mes=`grep -Po "$mes\/\d+:\d+:\d+:\d+\s\S+\]" $1 | wc -l | sed -e 's/^[[:space:]]*//'` # El número de accesos en Enero. Lo obtenemos usando una expresión regular, contamos el número de líneas que coinciden, y el sed elimina los espacios que aparecen al principio del número. En este caso no hace falta que usemos awk para eliminar el nombre del archivo, porque cuando en wc se lee desde stdin no se indica nada más que el número.
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"` # El número de accesos por hora lo obtenemos usando bc (basic calculator), porque no se pueden realizar operaciones aritméticas con números en punto flotante directamente.
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
mes='Jan' # Se repite el código para cada mes, sería más apropiado usar un for
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'`
accesos_mes=`grep -Po "$mes\/\d+:\d+:\d+:\d+\s\S+\]" $1 | wc -l | sed -e 's/^[[:space:]]*//'`
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"` 
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
mes='Feb' # Se repite el código para cada mes, sería más apropiado usar un for
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'`
accesos_mes=`grep -Po "$mes\/\d+:\d+:\d+:\d+\s\S+\]" $1 | wc -l | sed -e 's/^[[:space:]]*//'`
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"` 
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
fi
