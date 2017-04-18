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
	int fd, port, groupsize, messages;
    std::string host;
	struct sockaddr_in group_address, proc_address, sender_address;
    struct ip_mreq mreq;
};

/* Global Variables. */
int id, clock_count;
bool ordered;

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

/* Thread function that handles the sending of the multicast message. */
void *sender(void *args)
{
    /* Get the socket data. */
    struct socket_data *data;
    data = (struct socket_data *) args;

    /* Setup the port and the group address. */
    memset(&(data->group_address), 0, sizeof(data->group_address));
    data->group_address.sin_family = AF_INET;
    data->group_address.sin_addr.s_addr = inet_addr(data->host.c_str());
    data->group_address.sin_port = htons(data->port);

    /* Variables for the sender thread. */
    std::string buffer_str;
    int status;

    while(1)
    {
        sleep(2);
        log(1, " Sending multicast...");
        buffer_str = " Message from " + itos(id) + " at time " + itos(clock_count);
        clock_count++;
        status = sendto(data->fd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&(data->group_address), sizeof(data->group_address));
        if(status < 0)
        {
            log(1, " Error writing to the UDP socket. Exiting...");
            exit(1);
        }

        /* Send a multicast message to everyone in the group. */
        data->messages = 0;
    }
}

/* Thread function that handles the listening to the other's messages. */
void *receiver(void *args)
{
    /* Get the socket data. */
    struct socket_data *data;
    data = (struct socket_data *) args;

    /* Setup the address for the receiver. */
    memset(&(data->proc_address), 0, sizeof(data->proc_address));
    data->proc_address.sin_family = AF_INET;
    data->proc_address.sin_addr.s_addr = htonl(INADDR_ANY);
    data->proc_address.sin_port = htons(data->port);

    /* Bind to the receiving address. */
    int status = bind(data->fd, (struct sockaddr *)&(data->proc_address), sizeof(data->proc_address));
    if(status < 0)
    {
        log(1, " Error binding to the UDP socket. Exiting...");
        exit(1);
    }

    /* Set the ip_mreq information to join the multicast group. */
    data->mreq.imr_multiaddr.s_addr = inet_addr(data->host.c_str());
    data->mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    status = setsockopt(data->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &(data->mreq), sizeof(data->mreq));
    if(status < 0)
    {
        log(1, " Error joining the multicast group. Exiting...");
        exit(1);
    }

    /* Variables for the thread. */
    socklen_t addr_length = sizeof(data->sender_address);
    char buffer[MAX_DATASIZE];
    std::string buffer_str;
    
    while(1)
    {
        /* Wait to receive message from other processes. */
        status = recvfrom(data->fd, &buffer, MAX_DATASIZE, 0, (struct sockaddr *)&(data->sender_address), &addr_length);
        if(status < 0)
        {
            log(1, " Error receiving message from multicast. Exiting...");
            exit(1);
        }
        clock_count++;

        /* Print the message received from the multicast. */
        buffer[status] = '\0';
        buffer_str = buffer;
        if(ordered)
        {
            log(0, " Received message: " + buffer_str);
        }

        /* Send an acknowledgment to the sending process. */
        // buffer_str = "ACK";
        // if(sendto(data->fd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&(data->sender_address), sizeof(data->sender_address)) < 0)
        // {
        //     log(1, " Error writing to socket. Exiting...");
        //     exit(1);
        // }
        // clock_count++;
    }
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

    /* Check if the ordering needs to be maintained. */
    if(argc == 4)
        ordered = false;
    else
        ordered = true;

    /* Set the clock to a random value with PID as seed. */
	id = getpid();
	clock_count = 0;
    log(0, " Process started.");

    /* Set up the socket data and address. */
    struct socket_data *data;
	data = (struct socket_data*) malloc(sizeof(struct socket_data));
    data->host = argv[1];
    data->port = atoi(argv[2]);
    data->groupsize = atoi(argv[3]);

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

    /* Setup the thread variables. */
    pthread_t send, receive;
    pthread_create(&send, NULL, sender, data);
    pthread_create(&receive, NULL, receiver, data);

    /* Joing the two thread. */
    pthread_join(send, NULL);
    pthread_join(receive, NULL);

    close(data->fd);
    return 0;
}