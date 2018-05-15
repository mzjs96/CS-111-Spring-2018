#! /usr/local/cs/bin/gnuplot

set terminal png
set datafile separator ","

set title "Lab2b-1: Throughput vs. Number of Threads for Each Synchronization Method"
set xlabel "Iterations"
set logscale x 2
set xrange[0.5:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set key right top
set output 'lab2b_1.png'

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with Mutex' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with Spin-lock' with linespoints lc rgb 'blue'


# wait for lock times and per operations times
set title "Lab2b-2: Wait-for-lock Times and Per-operation Times for Mutex-synchronized List Operations"
set xlabel "Threads"
set logscale x 2
set xrange[0.5:]
set ylabel "Mean Time / Operation (ns)"
set logscale y 10
set key left top
set output 'lab2b_2.png'

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'Completion Time' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'Wait-for-Lock Time' with linespoints lc rgb 'red'


set title "Lab2b-3: Successful Iterations vs. Threads for Each Synchronization Method"
set xlabel "Threads"
set logscale x 2
set xrange[0.5:]
set ylabel "Successful Iterations"
set logscale y 10
set key left top
set output 'lab2b_3.png'

plot \
	 "< grep -e 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'No Protection' with points lc rgb 'red', \
     "< grep -e 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'with Mutex' with points lc rgb 'green', \
     "< grep -e 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'with Spin-lock' with points lc rgb 'blue'


set title "Lab2b-4: Throughput for Mutex Synchronized Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange[0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set key right top
set output 'lab2b_4.png'

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'orange', \
	 "< grep -e 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
	 "< grep -e 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'green'

set title "Lab2b-5: Throughput for Spin-Lock Synchronized Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange[0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set key right top
set output 'lab2b_5.png'

plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'orange', \
	 "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
	 "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'green'
