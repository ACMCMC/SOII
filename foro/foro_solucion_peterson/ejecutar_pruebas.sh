min=10
max=5000
incremento=10

echo 'M,T,errores' > estadisticas.txt
make
for (( i=$min; i<=$max; i+=$incremento ));
do
suma=0
for (( j=0; j<=$i; j++ ));
do
suma=$(( $suma + $j ))
done
echo "Probando $i"
./sumas_hilos.out $i 2 $suma >> estadisticas.txt
done