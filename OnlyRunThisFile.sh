g++ -o myserver server.cpp handleClient.cpp -lpthread
g++ client.cpp -o client
port=2222
thread_pool=12
./myserver $port $thread_pool &
max_load=15
for((load=20; load<max_load; load=$load+20));do
    mkdir $load
    for ((i = 0; i < $load; i++)); do
        ./client localhost $port write.c 15 1 > "$load/$i$load.txt"
        wait_pid[${i}]=$!
    done
    vmstat 1 | awk '{print $15}' >> cpu-utilization$load.txt &
    for pid in ${wait_pid[*]};do
            wait $pid
    done
    killall vmstat 
done
for((myload=20; myload<max_load; myload=$myload+20));do
	cat cpu-utilization$myload.txt | awk -v loads=$myload 'BEGIN { sum=0; count=0; }
/^[0-9]+$/ { sum+=$0; count++; }
END { if (count > 0) { print loads,(100-sum/count); } }' >>mvsload.txt
done

for((myload=20; myload<max_load; myload=$myload+20));do
    cd $myload
    file_count=$(ls | wc -l) 
    grep "Request Rate Sent" *.txt | awk -v file_count="$file_count" -v loads="$myload" 'BEGIN{ sum=0; } {sum += $4;} END { print loads, (sum/file_count) }' >> ../MvsRequestRate.txt
   cd ..
done
#generating Clients vs CPU Utilization
input_file="mvsload.txt"
output_file="ClientVsCPU.png"

gnuplot <<EOF
set term png size 800,600
set output "$output_file"
set title "MVS Load"
set xlabel "Clients"
set ylabel "Cpu Utilization"
plot "$input_file" using 1:2 with lines
EOF

input_file="MvsRequestRate.txt"
output_file="ClientVsRequestRate.png"

gnuplot <<EOF
set term png size 800,600
set output "$output_file"
set title "M vs Request Rate Load"
set xlabel "Clients"
set ylabel "Request Rate Load"
plot "$input_file" using 1:2 with lines
EOF
