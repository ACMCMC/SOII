#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
check_args() {
    [[ $1=='-a' && $2 ]] || [[ $1=='-s' && $2 && $3 ]] || [[ $1=='-e' && $2 && $3 ]] || [[ $1=='-u' && $2 && $3 ]]
    return $?
}
if (check_args $1 $2 $3);
then
    case $1 in
    '-a')
    cat $2/sala_??_*.??? > $2/salida.out
    ;;
    '-s')
    cat $3/sala_$2_*.??? > $3/salida.out
    ;;
    '-e')
    cat $3/sala_??_*.$2 > $3/salida.out
    ;;
    '-u')
    cat $3/sala_??_$2.??? > $3/salida.out
    ;;
    esac
else
echo "Formato: $0 [opción] [parámetro]"
fi