#	Sabin Raj Tiwari
#	CMSC 621
#	Project 2
#	Assignment 3

HOST=$1
PORT=$2
GROUPSIZE=$3

# Start the sequencer
./part_three $HOST $PORT $GROUPSIZE "Y" &

# Start client processes.
for ((a = 1; a < $GROUPSIZE; a++))
do
    sleep 0.1s
    ./part_three $HOST $PORT $GROUPSIZE &
done

# Wait for the programs to converge.
wait