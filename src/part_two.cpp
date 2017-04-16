/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 2
*/

/* Include header files. */
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Main method to execute the program logic. */
int main(int argc, char **argv)
{
    if(argc < 3)
	{
		/* Show error if the correct number of arguments were not passed. */
		cerr << "Usage: part_two <group_address> <port_number> <groupsize>\n";
		exit(1);
	}
}