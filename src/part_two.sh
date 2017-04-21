#	Sabin Raj Tiwari
#	CMSC 621
#	Project 2
#	Assignment 2

HOST=$1
PORT=$2
GROUPSIZE=$3
ORDERED=$4

# Start the sequencer
if [ $ORDERED == 'Y' ]
then
	./part_two $HOST $PORT $GROUPSIZE "Y" "Y" &
else
	./part_two $HOST $PORT $GROUPSIZE &
fi

# Start client processes.
for ((a = 1; a < $GROUPSIZE; a++))
do
	if [ $ORDERED == 'Y' ]
	then
		./part_two $HOST $PORT $GROUPSIZE "Y" &
	else
		./part_two $HOST $PORT $GROUPSIZE &
	fi
done

# Wait for the programs to converge.
wait