/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 2
*/

/* Include header files. */
#include <arpa/inet.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* Define global constants. */
#define MAX_DATASIZE 4096
#define GROUP_HOST "224.0.0.1"
#define GROUP_PORT 3000
#define GROUP_SIZE 5
#define MESSAGES 25

/* Import namespaces. */
using namespace std;

/* Structure for the program. */
struct conn_data
{
	int fd, port, groupsize, messages;
    std::string host;
};

/* Global Variables. */
int id, clock_count, last_recv;
bool ordered;
std::ofstream log_file;

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
    // time_t timestamp = time(0);
    // char *dt = ctime(&timestamp);
    // std::string dt_str(dt);
    // dt_str = dt_str.substr(0, dt_str.size() - 1);

    time_t rawtime;
    struct tm * timeinfo;
    char timebuffer[80];
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (timebuffer, 80, "%I:%M:%S", timeinfo);

    struct timeval current;
    gettimeofday(&current, 0);
    int milli = current.tv_usec;

    std::string dt_str(timebuffer);
    dt_str = dt_str + ":" + itos(milli / 10);

    //std::string dt_str = "";
    if(log_file.is_open())
        log_file << "[" << dt_str << "][Process " << id << "]" << message << endl;
	if(type == 1) 
        cerr << "[" << dt_str << "][Process " << id << "]" << message << "\n";
	else 
        cout << "[" << dt_str << "][Process " << id << "]" << message << "\n";		
}

/* Function that handles the timeout for the recv calls for 4 seconds. */
void *timer(void *args)
{
    /* Get the connection data. */
    struct conn_data *data;
    data = (struct conn_data *) args;

    int end = 0;
    while(end == 0)
    {
        sleep(1);
        last_recv++;
        if(last_recv >= 4)
        {
            end = 1;
        }
    }

    /* Exit the program when the timeout is reached. */
    log(0, " Exiting...");
    log_file.close();
    close(data->fd);
    exit(0);
}

/* Thread function that handles the sending of the multicast message. */
void *sender(void *args)
{
    /* Get the connection data. */
    struct conn_data *data;
    data = (struct conn_data *) args;

    /* Setup the port and the group address. */
    struct sockaddr_in group_address;
    memset(&group_address, 0, sizeof(group_address));
    group_address.sin_family = AF_INET;
    group_address.sin_addr.s_addr = inet_addr(data->host.c_str());
    group_address.sin_port = htons(data->port);

    /* Variables for the sender thread. */
    std::string buffer_str;
    int status;

    /* Send a message for the count of MESSAGES. */
    //sleep(2);
    for(int i = 0; i < MESSAGES; i++)
    {
        clock_count++;
        buffer_str = itos(id) + ":" + itos(clock_count);
        status = sendto(data->fd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&group_address, sizeof(group_address));
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
    /* Get the connection data. */
    struct conn_data *data;
    data = (struct conn_data *) args;

    /* Allow mutiple sockets to use same port. */
	uint status = 1;
	if(setsockopt(data->fd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) < 0)
	{
		log(1, " Error reusing port for UDP socket. Exiting...");
		exit(1);
	}

    /* Setup the address for the receiver. */
    struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(data->port);

    /* Bind to the receiving address. */
    status = bind(data->fd, (struct sockaddr *)&address, sizeof(address));
    if(status < 0)
    {
        log(1, " Error binding to the UDP socket. Exiting...");
        exit(1);
    }

    /* Set the ip_mreq information to join the multicast group. */
    struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(data->host.c_str());
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    status = setsockopt(data->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if(status < 0)
    {
        log(1, " Error joining the multicast group. Exiting...");
        exit(1);
    }

    /* Set the receive option to timeout if no messages are received within 3 seconds. */
    // struct timeval timeout;
    // timeout.tv_sec = 0;
    // timeout.tv_usec = 100000;
    // status = setsockopt(data->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    // if(status < 0) {
    //     perror("Error");
    // }

    /* Variables for the thread. */
    socklen_t addr_length = sizeof(address);
    char buffer[MAX_DATASIZE];
    std::string buffer_str;
    
    while(1)
    {
        /* Wait to receive message from other processes. */
        memset(&buffer[0], 0, MAX_DATASIZE);
        status = recvfrom(data->fd, &buffer, MAX_DATASIZE, 0, (struct sockaddr *)&address, &addr_length);
        if(status < 0)
        {
            log(1, " Exiting...");
            exit(1);
        }

        /* Filter the messages from itself. */
        last_recv = 0;
        buffer[status] = '\0';
        buffer_str = buffer;
        if(buffer_str.find(itos(id) + ":") == std::string::npos)
        {  
            /* Print the message received from the multicast. */
            clock_count++;
            if(!ordered)
            {
                log(0, " Received - " + buffer_str);
            }
        }
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
    last_recv = 0;

    /* Setup the log file. */
    std::string filename = "logs/process_" + itos(id) + "log.txt";
    log_file.open(filename.c_str(), ios::out | ios::trunc);
    log(0, " Process started.");

    /* Set up the connection data for the program. */
    struct conn_data *data;
	data = (struct conn_data*) malloc(sizeof(struct conn_data));
    data->host = argv[1];
    data->port = atoi(argv[2]);
    data->groupsize = atoi(argv[3]);

    data->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(data->fd < 0)
    {
        log(1, " Error creating UDP socket. Exiting...");
        exit(1);
    }

    /* Setup the thread variables. */
    pthread_t send, receive, timeout;
    pthread_create(&send, NULL, sender, data);
    pthread_create(&receive, NULL, receiver, data);  
    pthread_create(&timeout, NULL, timer, data);

    /* Joing the two thread. */
    pthread_join(send, NULL);
    pthread_join(receive, NULL);  
    pthread_join(timeout, NULL);  

    close(data->fd);
    log_file.close();
    return 0;
}