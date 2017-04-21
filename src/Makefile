#	Sabin Raj Tiwari
#	CMSC 621
#	Project 2

compile: part_one part_two part_three directories

part_one:
	g++ -o part_one part_one.cpp

part_two:
	g++ -std=c++11 -o part_two part_two.cpp -lpthread -lm

part_three:
	g++ -o part_three part_three.cpp -lpthread
	@echo "0" > part_three.txt

clean:
	rm -f part_one part_two part_three
	rm -f part_three.txt
	rm -rf logs

directories:
	mkdir -p logs