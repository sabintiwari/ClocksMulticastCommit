#	Sabin Raj Tiwari
#	CMSC 621
#	Project 2
#	Assignment 1

HOST=$1
PORT=$2
GROUPSIZE=$3

# Start client processes.
for ((a = 1; a < $GROUPSIZE; a++))
do
	sleep 0.1s
	./part_one $HOST $PORT&
done

# Start the time daemon.
sleep 0.1s
./part_one $HOST $PORT $GROUPSIZE &

# Wait for the programs to converge.
wait 