/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
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
#include <time.h>
#include <unistd.h>

/* Define global constants. */
#define MAX_DATASIZE 1024

/* Import namespaces. */
using namespace std;

/* Structures for the program. */

struct client_data
{
	/* Structure that handles the sockets data for the client. */
	int socket_fd;
	struct sockaddr_in client_address;
};

/* Global Variables. */
int id, clock_count;

/* Get the string value from an int. */
std::string i_to_s(int value)
{
	std::stringstream str;
	str << value;
	return str.str();
}

/* Method that performs the time daemon functionality. */
void daemon(std::string host, int port, int groupsize)
{
	/* Create the datagram socket and exit if there is an error. */
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		cerr << "Error creating UDP socket. Time daemon exiting.\n";
		exit(0);
	}

	/* Setup the port and the group address. */
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(host.c_str());
	address.sin_port = htons(port);

	/* Send a poll to all the listening clients to send their clocks. */
	cout << "Time daemon started with clock " << clock_count << "...\n";
	int response = clock_count;
	int w = sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address));
	if(w < 0)
	{
		cerr << "Error writing to the socket. Time daemon exiting.\n";
		exit(0);
	}

	/* Loop for all the clients. */
	struct sockaddr_in in_address;
	socklen_t address_length;
	int r, messages = 0, sum = 0;
	while(messages < groupsize - 1)
	{
		/* Read from the clients. */
		address_length = sizeof(in_address);
		r = recvfrom(fd, &response, sizeof(int), 0, (struct sockaddr *)&in_address, &address_length);
		if(r < 0)
		{
			cerr << "Error receiving message from sender. Time daemon exiting.\n";
			exit(1);
		}
		messages++;
		sum+= response;
	}

	/* Calculate the clock average and update the count for self and send the updated clocks count. */
	clock_count+= (sum / groupsize);
	cout << "Time daemon's clock has been updated to: " << clock_count << "\n";
	sendto(fd, &clock_count, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address));
	if(w < 0)
	{
		cerr << "Error writing to the socket. Time daemon exiting.\n";
		exit(0);
	}

	/* Wait for acknowledgements from the clients that updates were done before closing socket. */
	messages = 0;
	while(messages < groupsize - 1)
	{
		/* Read from the clients. */
		address_length = sizeof(address);
		r = recvfrom(fd, &response, sizeof(int), 0, (struct sockaddr *)&in_address, &address_length);
		if(r < 0)
		{
			cerr << "Error receiving message from sender. Time daemon exiting.\n";
			exit(1);
		}
		messages++;
	}

	close(fd);
}

/* Method that performs the function of the client machines that need to be synchronised. */
void client(std::string host, int port)
{
	int fd, b, s, w, r, y;
	
	/* Create the datagram socket and exit if there is an error. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		cerr << "Error creating UDP socket. Process " << id << " exiting.\n";
		exit(0);
	}

	/* Allow mutiple sockets to use same port. */
	y = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
	if(y < 0)
	{
		cerr << "Error reusing port for UDP socket. Process " << id << " exiting.\n";
		exit(0);
	}

	/* Set up the sender addreses. */
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	/* Bind to the receiving address. */
	b = bind(fd, (struct sockaddr *)&address, sizeof(address));
	if(b < 0)
	{
		cerr << "Error binding to UDP socket. Process " << id << " exiting.\n";
		exit(1);
	}

	/* Set the ip_mreq information to join the multicast group. */
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(host.c_str());
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	s = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if(s < 0)
	{
		cerr << "Error joining multicast group. Process " << id << " exiting.\n";
		exit(1);
	}

	/* Read from the daemon. */
	cout << "Process " << id << " started with clock " << clock_count << "...\n";
	socklen_t address_length = sizeof(address);
	int daemon_time;
	r = recvfrom(fd, &daemon_time, sizeof(int), 0, (struct sockaddr *)&address, &address_length);
	if(r < 0)
	{
		cerr << "Error receiving message from sender. Process " << id << " exiting.\n";
		exit(1);
	}

	/* Send the clock count to the daemon. */
	int response = clock_count - daemon_time;
	cout << "Process " << id << " is sending: " << response << "\n";
	w = sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address));
	if(w < 0)
	{
		cerr << "Error writing to the socket. Process " << id << " exiting.\n";
		exit(0);
	}

	/* Wait for the server to send the updated clock. */
	r = recvfrom(fd, &daemon_time, sizeof(int), 0, (struct sockaddr *)&address, &address_length);
	if(r < 0)
	{
		cerr << "Error receiving message from sender. Process " << id << " exiting.\n";
		exit(1);
	}

	/* Set the client time to the updated time. */
	clock_count = daemon_time;
	cout << "Process " << id << " has been updated to: " << clock_count << "\n";

	/* Send the response to the time daemon that the clock has been updated. */
	response = 1;
	w = sendto(fd, &response, sizeof(int), 0, (struct sockaddr *)&address, sizeof(address));
	if(w < 0)
	{
		cerr << "Error writing to the socket. Process " << id << " exiting.\n";
		exit(0);
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

	if(argc == 3)
	{
		/* Call the client method if 1 argument is passed. */
		client(argv[1], atoi(argv[2]));
	}
	else 
	{
		/* Call the daemon method if 2 arguments are passed. */
		daemon(argv[1], atoi(argv[2]), atoi(argv[3]));
	}

	return 0;
}