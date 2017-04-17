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
#include <pthread.h>
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

/* Structure for the program. */
struct socket_data
{
	int fd;
	struct sockaddr_in address, sender_address;
    struct ip_mreq mreq;
};

/* Global Variables. */
int id, clock_count;

/* Logs a message to cout or cerr. */
void log(int type, std::string message)
{
	if(type == 1)
		cerr << "[Process " << id << "]" + message + "\n";
	else
		cout << "[Process " << id << "]" + message + "\n";
}

/* Thread function that handles the sending of the multicast message. */
void *sender(void *args)
{

}

/* Thread function that handles the listening to the other's messages. */
void *receiver(void *args)
{

}

/* Main method to execute the program logic. */
int main(int argc, char **argv)
{
    if(argc < 4)
	{
		/* Show error if the correct number of arguments were not passed. */
        cerr << "User non-ordered: part_two <group_address> <port_number> <groupsize><\n";
		cerr << "Usage ordered: part_two <group_address> <port_number> <groupsize> <ordered>\n";
		exit(1);
	}

    /* Set the clock to a random value with PID as seed. */
	id = getpid();
	srand(id);
	clock_count = rand() % 100;

    /* Set up the socket data and address. */
    struct socket_data *data;
	data = (struct socket_data*) malloc(sizeof(struct socket_data));

    data->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(data->fd < 0)
    {
        log(1, " Error creating UDP socket. Exiting...");
        exit(1);
    }

    /* Allow mutiple sockets to use same port. */
	uint status = 1;
	if(setsockopt(data->fd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) < 0)
	{
		log(1, " Error reusing port for UDP socket. Exiting...");
		exit(1);
	}

    /* Set up the address. */
    memset(&(data->address), 0, sizeof(data->address));
    data->address.sin_family = AF_INET;
    data->address.sin_addr.s_addr = htonl(INADDR_ANY);
    data->address.sin_port = htons(atoi(argv[2]));
    
    /* Bind the socket to the address. */
    if(bind(data->fd, (struct sockaddr *)&(data->address), sizeof(data->address)) < 0)
	{
		log(1, " Error binding to the UDP socket. Exiting...");
		exit(1);
	}

    /* Set the ip_mreq information to join the multicast group. */
	data->mreq.imr_multiaddr.s_addr = inet_addr(argv[1]);
	data->mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if(setsockopt(data->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &(data->mreq), sizeof(data->mreq)) < 0)
	{
		log(1, " Error joining the multicast group. Exiting...");
		exit(1);
	}

    /* Setup the thread variables. */
    pthread_t send, receive;
    pthread_create(&send, NULL, sender, data);
    pthread_create(&receive, NULL, receiver, data);

    if(argc == 4)
    {
        /* Call the non ordered function if 4 arguments are passed. */

    }
    else
    {
        /* Call the ordered function if 5 arguments are passed. */

    }

    /* Joing the two thread. */
    pthread_join(send, NULL);
    pthread_join(receive, NULL);

    return 0;
}