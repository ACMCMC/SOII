#!/bin/bash
if [ $# -eq 0 ]; then
echo "No se ha definido la ruta del archivo. Uso: $0 [ruta]"
else
grep -Eo '^(?:\d{1,3}.){3}\d{1,3}' $1 | sort | uniq -c > /tmp/access_ip.log
grep -Eo '\S+\s\S+"\s404\s\d+$' $1 | grep -Eo '^\S+' > /tmp/access_404.log
num_ips=`wc -l /tmp/access_ip.log`
num_404=`wc -l /tmp/access_404.log`
num_accesos=`wc -l $1 | awk 'END {print $1}'`
echo "$num_ips direcciones diferentes extraídas en /tmp/access_ip.log"
echo "$num_404 accesos a URLs no encontradas extraídos en /tmp/access_404.log"
echo "$num_accesos accesos en total:"
mes='Dec'
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'`
accesos_mes=`grep -Eo '\w+\/\d+:\d+:\d+:\d+\s\S+\]' $1 | grep -Eo "^$mes" | wc -l | sed -e 's/^[[:space:]]*//'`
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"`
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
mes='Jan'
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'`
accesos_mes=`grep -Eo '\w+\/\d+:\d+:\d+:\d+\s\S+\]' $1 | grep -Eo "^$mes" | wc -l | sed -e 's/^[[:space:]]*//'`
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"`
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
mes='Feb'
dias_por_mes=`cal -m $mes | awk 'NF {DAYS = $NF}; END {print DAYS}'`
accesos_mes=`grep -Eo '\w+\/\d+:\d+:\d+:\d+\s\S+\]' $1 | grep -Eo "^$mes" | wc -l | sed -e 's/^[[:space:]]*//'`
accesos_por_hora=`bc <<< "scale=1; $accesos_mes / ( $dias_por_mes * 24 )"`
echo "$mes: $accesos_mes accesos en total ($accesos_por_hora accesos por hora)"
fi