#!/bin/bash
# Aldán Creo Mariño, SOII 20/21
check_shell() { # Función que solo debe recibir como parámetro el nombre del shell para hacer la búsqueda
    return `grep -Fq $1 /etc/shells` # Devuelve verdadero para una línea que contiene parcialmente el nombre; por ejemplo, si existe '/bin/bash', y ponemos 'a', devuelve verdadero
}
echo "Nombre de shell especificado: $1" # Info para el usuario
if (check_shell $1) # Comprobamos que el shell existe
then
grep $1 /etc/passwd # Buscamos las líneas que contengan esa palabra en el archivo /etc/passwd
else
echo "Se ha especificado un nombre de shell que no existe" # Mensaje de error
fi