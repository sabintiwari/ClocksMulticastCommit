/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 1
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

/* Define global constants. */
#define MAX_DATASIZE 1024
#define GROUP_HOST "224.0.0.1"
#define GROUP_PORT 3000
#define GROUP_SIZE 5

/* Import namespaces. */
using namespace std;

/* Global Variables. */
int id, clock_count;

/* Get the string value from an int. */
std::string itos(int value)
{
	std::stringstream str;
	str << value;
	return str.str();
}

/* Logs a message to cout or cerr. */
void log(int type, std::string message)
{
	if(type == 1)
		cerr << "[Process " << id << "]" + message + "\n";
	else
		cout << "[Process " << id << "]" + message + "\n";
}

/* Method that performs the time daemon functionality. */
void daemon(std::string host, int port, int groupsize)
{
	/* Create the datagram socket and exit if there is an error. */
	sleep(1);
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		log(1, "[Time Daemon] Error creating UDP socket. Exiting...");
		exit(1);
	}

	/* Setup the port and the group address. */
	struct sockaddr_in group_address;
	memset(&group_address, 0, sizeof(group_address));
	group_address.sin_family = AF_INET;
	group_address.sin_addr.s_addr = inet_addr(host.c_str());
	group_address.sin_port = htons(port);

	/* Send a poll to all the listening clients to send their clocks. */
	log(0, "[Time Daemon] Started. Current clock: " + itos(clock_count));
	int response = clock_count;
	if(sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&group_address, sizeof(group_address)) < 0)
	{
		log(1, "[Time Daemon] Error writing to the UDP socket. Exiting...");
		exit(1);
	}

	/* Loop for all the clients. */
	struct sockaddr_in in_address;
	socklen_t address_length = sizeof(in_address);
	int messages = 0, sum = 0;
	while(messages < groupsize - 1)
	{
		/* Read from the clients. */
		if(recvfrom(fd, &response, sizeof(int), 0, (struct sockaddr *)&in_address, &address_length) < 0)
		{
			log(1, "[Time Daemon] Error receiving message. Exiting...");
			exit(1);
		}
		messages++;
		sum+= response;
	}

	/* Calculate the clock average and update the count for self and send the updated clocks count. */
	clock_count+= (sum / groupsize);
	log(0, "[Time Daemon] Clock updated: " + itos(clock_count));
	if(sendto(fd, &clock_count, sizeof(int), 0, (struct sockaddr *)&group_address, sizeof(group_address)) < 0)
	{
		log(1, "[Time Daemon] Error writing multicast. Exiting...");
		exit(1);
	}

	/* Wait for acknowledgements from the clients that updates were done before closing socket. */
	messages = 0;
	while(messages < groupsize - 1)
	{
		/* Read from the clients. */
		address_length = sizeof(in_address);
		if(recvfrom(fd, &response, sizeof(int), 0, (struct sockaddr *)&in_address, &address_length) < 0)
		{
			log(1, "[Time Daemon] Error receiving message. Exiting...");
			exit(1);
		}
		messages++;
	}

	close(fd);
}

/* Method that performs the function of the client machines that need to be synchronised. */
void client(std::string host, int port)
{	
	/* Create the datagram socket. Exit if there is an error. */
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		log(1, " Error creating UDP socket. Exiting...");
		exit(1);
	}

	/* Allow mutiple sockets to use same port. */
	uint status = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) < 0)
	{
		log(1, " Error reusing port for UDP socket. Exiting...");
		exit(1);
	}

	/* Set up the address. */
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	/* Bind to the receiving address. */
	if(bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		log(1, " Error binding to the UDP socket. Exiting...");
		exit(1);
	}

	/* Set the ip_mreq information to join the multicast group. */
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(host.c_str());
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		log(1, " Error joining the multicast group. Exiting...");
		exit(1);
	}

	/* Read from the daemon. */
	log(0, " Started. Current clock: " + itos(clock_count));
	int daemon_time;
	socklen_t address_length = sizeof(address);
	if(recvfrom(fd, &daemon_time, sizeof(int), 0, (struct sockaddr *)&address, &address_length) < 0)
	{
		log(1, " Error receiving message from multicast. Exiting...");
		exit(1);
	}

	/* Send the clock count to the daemon. */
	int response = clock_count - daemon_time;
	log(0, " Sending: " + itos(response));
	if(sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		log(1, " Error writing to socket. Exiting...");
		exit(1);
	}

	/* Wait for the server to send the updated clock. */
	address_length = sizeof(address);
	if(recvfrom(fd, &daemon_time, sizeof(int), 0, (struct sockaddr *)&address, &address_length) < 0)
	{
		log(1, " Error receiving message from multicast. Exiting...");
		exit(1);
	}

	/* Set the client time to the updated time. */
	clock_count = daemon_time;
	log(0, " Clock updated: " + itos(clock_count));

	/* Send the response to the time daemon that the clock has been updated. */
	response = 1;
	if(sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		log(1, " Error writing to socket. Exiting...");
		exit(1);
	}

	close(fd);
}

/* Main method to execute the program logic. */
int main(int argc, char **argv)
{
	if(argc < 3)
	{
		/* Show error if the correct number of arguments were not passed. */
		cerr << "Usage time daemon: part_one <group_address> <port_number> <groupsize>\n";
		cerr << "Usage clients: part_one <group_address> <port_number>\n";
		exit(1);
	}

	/* Set the clock to a random value with PID as seed. */
	id = getpid();
	srand(id);
	clock_count = rand() % 100;

	/* Verify that the address and port are passed. */
	std::string host = argv[1];
	std::string port_str = argv[2];
	int port;
	if(host.empty())
		host = GROUP_HOST; /* Set the host to default. */
	if(port_str.empty())
		port = GROUP_PORT; /* Set the port to default. */
	else
		port = atoi(port_str.c_str());

	if(argc == 3)
	{
		/* Call the client method if 2 arguments are passed. */
		client(host, port);
	}
	else 
	{
		/* Call the daemon method if 3 arguments are passed. */
		std::string groupsize_str = argv[3];
		int groupsize;
		if(groupsize_str.empty())
			groupsize = GROUP_SIZE; /* Set the group size to default. */
		else
			groupsize = atoi(groupsize_str.c_str());
		daemon(host, port, groupsize);
	}

	return 0;
}