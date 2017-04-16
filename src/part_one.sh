#	Sabin Raj Tiwari
#	CMSC 621
#	Project 2

HOST=$1
PORT=$2
CLIENTS=5

# Start 5 clients.
for ((a = 1; a <= $CLIENTS; a++))
do
	sleep 0.1s
	./part_one $HOST $PORT&
done
# Start the time daemon.
./part_one $HOST $PORT $CLIENTS
# Wait for the programs to converge.
wait