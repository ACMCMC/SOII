#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
check_args() {
    [[ $1=='-a' && $2 ]] || [[ $1=='-s' && $2 && $3 ]] || [[ $1=='-e' && $2 && $3 ]] || [[ $1=='-u' && $2 && $3 ]] # Hacemos un test: las opciones necesarias están definidas?
    return $? # Devolvemos el estado del comando anterior, que es un número (TRUE/FALSE)
}
if (check_args $1 $2 $3); # Ejecutamos la función de comprobación
then
    case $1 in # Comprobamos cuál es el primer argumento
    '-a')
    cat $2/sala_??_*.??? > $2/salida.out # Unimos todos los vídeos. Esto lo hacemos con un cat de todos los archivos, redirigiendo la salida a salida.out
    ;;
    '-s')
    cat $3/sala_$2_*.??? > $3/salida.out # Unimos todos los vídeos que pertenecen a la sala indicada
    ;;
    '-e')
    cat $3/sala_??_*.$2 > $3/salida.out # Unimos todos los vídeos con la extensión especificada
    ;;
    '-u')
    cat $3/sala_??_$2.??? > $3/salida.out # Unimos todos los vídeos que pertenecen al usuario elegido
    ;;
    esac
else
echo "Formato: $0 [opción] [parámetro]" # Mensaje de error
fi
