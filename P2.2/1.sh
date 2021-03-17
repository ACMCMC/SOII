#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
if [ $# -eq 0 ]
then
echo "Formato: $0 [fichero]" # Mensaje de error
else
echo "Se ha especificado el nombre de archivo $1" # Mensaje para el usuario
if (test -f $1) # Miramos si el fichero existe y es de tipo regular
then
cp $1 /tmp # Copiamos el fichero a /tmp
else
fecha=`date +"%b %d %T"` # Obtenemos la fecha actual
nombre_equipo=`hostname` # El comando hostname devuelve el nombre del equipo
echo "$fecha $nombre_equipo $1: Fichero no encontrado" >> error.log # Añadimos una línea al archivo error.log
fi
fi