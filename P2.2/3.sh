#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
check_args() {
    return `test \($1='-a' -a $2\) -o \($1='-s' -a $2\)`
}
if (check_args $1 $2 $3)
then
    case $1 in
    '-a')
    cat $2/sala_??_*.??? > salida.out
    ;;
    '-s')
    cat $3/sala_??_*.??? > salida.out
    ;;
    '-e')
    cat $3/sala_??_*.$2 > salida.out
    ;;
    '-u')
    cat $3/sala_??_$2.??? > salida.out
    ;;
    esac
else
echo "Formato: $0 [opción] [parámetro]"
fi