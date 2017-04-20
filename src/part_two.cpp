/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 2
*/

/* Include header files. */
#include <algorithm>
#include <arpa/inet.h>
#include <ctime>
#include <fstream>
#include <iterator>
#include <iostream>
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
#include <vector>

/* Define global constants. */
#define MAX_DATASIZE 4096
#define GROUP_HOST "224.0.0.1"
#define GROUP_PORT 3000
#define GROUP_SIZE 5
#define MESSAGE_COUNT 5

/* Import namespaces. */
using namespace std;

/* Structures for the program. */
struct conn_data
{
	int fd, port, groupsize, messages;
    std::string host;
};
struct message
{
    int process, sendclock, selfclock;
    bool delivered, isseqmsg;
    std::string str;
};

/* Global Variables. */
int id, clock_count, last_recv, recv_count;
bool ordered, sequencer;
std::vector<struct message> message_vector;
std::ofstream log_file;

/* Get the string value from an int. */
std::string itos(int value)
{
    std::stringstream str;
    str << value;
    return str.str();
}

/* Splits a string using a delimeter. */
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

/* Uses split() to get the vector of string elements. */
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

/* Logs a message to cout or cerr. */
void log(int type, std::string message)
{
    /* Get the Hour, Minute, and Seconds timestamp. */
    time_t rawtime;
    struct tm * timeinfo;
    char timebuffer[80];
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (timebuffer, 80, "%I:%M:%S", timeinfo);

    /* Get the nanosecond timestamp */
    struct timeval current;
    gettimeofday(&current, 0);
    int nano = current.tv_usec;

    /* Create the log message */
    std::string dt_str(timebuffer);
    dt_str = dt_str + ":" + itos(nano);
    std::string stamp = "[" + dt_str + "][" + itos(id) + "]";
    if(sequencer)
    {
        stamp = stamp.append("[Sequencer]");
    }
    else
    {
        stamp = stamp.append("[Process]");
    }
        
    if(log_file.is_open())
    {
        /* Write to the log file */
        log_file << stamp << " " << message << endl;
    }
        
	if(type == 1)
    {
        /* Log to cerr if its an error. */
        cerr << stamp << " " << message << "\n";
    }
	else 
    {
        /* Log to cout if its a regular message. */
        cout << stamp << " " << message << "\n";
    }		
}

/* Checks if the message can be printed. */
bool is_printable(message a)
{
    if(a.process == id) return false;

    std::string msg;
    bool print = false;
    int index = -1;
    message b;
    /* Iterate through the messages and check if there exists a sequencer or a process message.*/
    for(int i = 0; i < message_vector.size(); i++)
    {
        b = message_vector.at(i);
        if(!a.isseqmsg && b.isseqmsg)
        {
            /* If the message to check is a client message, then check for a corresponding sequencer message. */
            if(b.sendclock == recv_count + 1) {
                msg = itos(a.process) + ":" + itos(a.sendclock) + ":" + a.str;
                if(msg == b.str)
                {
                    /* If the message matches for the seq and client message.*/
                    print = true;
                    index = i;
                    recv_count++;
                }
            }
        }
        else if(a.isseqmsg && !b.isseqmsg)
        {
            /* If the message to check is a sequence message, then check for a corresponding sequencer message. */
            if(a.sendclock == recv_count + 1) {
                msg = itos(b.process) + ":" + itos(b.sendclock) + ":" + b.str;
                if(msg == a.str)
                {
                    /* If the message matches for the seq and client message.*/
                    print = true;
                    index = i;
                    recv_count++;
                }
            }
        }
    }

    /* Remove the sequencer message. */
    if(print && index > -1) {
        log(0, "Removing index " + itos(index) + ". Size: " + itos(message_vector.size()));        
        message_vector.erase(message_vector.begin() + index);
        log(0, "Removed index " + itos(index) + ". Size: " + itos(message_vector.size()));   
    }

    return print;
}

/* Function that inserts the message at the next possible index based on sort. */
bool insert_message(message a) 
{
    bool isadded = false;
    message b;
    for(int i = 0; i < message_vector.size(); i++)
    {  
        /* Iterate and check where the item needs to be added then add. */
        b = message_vector.at(i);
        if(a.process == b.process && a.sendclock < b.sendclock) {
            message_vector.insert(message_vector.begin() + i, a);
            isadded = true;
        }
    }

    if(!isadded)
    {
        /* If the item has not been added, add anyway. */
        message_vector.push_back(a);
        isadded = true;
    }

    return isadded;
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
        if((!sequencer && last_recv >= 5) || (sequencer && last_recv >= 7))
        {
            end = 1;
        }
    }

    /* Print the buffered messages if the timeout was reached. */
    message a;
    for(int i = 0; i < message_vector.size(); i++) {
        a = message_vector.at(i);
        if(a.isseqmsg)
            log(0, "Delivered - " + a.str);
        else
            log(0, "Delivered - " + itos(a.process) + ":" + itos(a.sendclock) + ":" + a.str);
    }

    /* Exit the program when the timeout is reached. */
    log(0, "Exiting...");
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
    message msg;

    /* Send a message for the count of MESSAGE_COUNT. */
    sleep(3);
    if(sequencer)
    {
        sleep(1);
        /* If the process is a sequencer, send the sequencing message. */
        while(1)
        {
            if(message_vector.size() > 0)
            {
                msg = message_vector.at(0);
                clock_count++;
                buffer_str = "SEQ:" + itos(clock_count) + ":" + itos(msg.process) + ":" + itos(msg.sendclock) + ":" + msg.str;
                
                /* Send the sequencer message. */
                log(0, "Sending - " + buffer_str);
                status = sendto(data->fd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&group_address, sizeof(group_address));
                if(status < 0)
                {
                    log(1, "Error writing to the UDP socket. Exiting...");
                    exit(1);
                }
                
                /* Remove the message from the list after sending. */
                message_vector.erase(message_vector.begin());
            }
        }
    }
    else
    {
        /* If the process is NOT a sequencer, send the regular multicast. */
        for(int i = 0; i < MESSAGE_COUNT; i++)
        {
            clock_count++;
            buffer_str = itos(id) + ":" + itos(clock_count) + ":" + "MESSAGE_" + itos(i + 1);
            /* Send the regular multicast message. */
            log(0, "Sending - " + buffer_str);
            status = sendto(data->fd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&group_address, sizeof(group_address));
            if(status < 0)
            {
                log(1, "Error writing to the UDP socket. Exiting...");
                exit(1);
            }
        }
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
		log(1, "Error reusing port for UDP socket. Exiting...");
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
        log(1, "Error binding to the UDP socket. Exiting...");
        exit(1);
    }

    /* Set the ip_mreq information to join the multicast group. */
    struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(data->host.c_str());
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    status = setsockopt(data->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if(status < 0)
    {
        log(1, "Error joining the multicast group. Exiting...");
        exit(1);
    }

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
            log(1, "Exiting...");
            exit(1);
        }

        /* Filter the messages from itself. */
        last_recv = 0;
        buffer[status] = '\0';
        buffer_str = buffer;
        if(buffer_str.find(itos(id) + ":") == std::string::npos)
        {  
            if(!ordered)
            {
                /* Print the message received from the multicast. */
                clock_count++;
                log(0, "Received and Delivered - " + buffer_str);
            }
            else
            {
                /* Store the message to send later. */
                message msg;
	            std::vector<std::string> tokens = split(buffer_str, ':');
                
                if(tokens.size() == 3)
                {
                    log(0, "Received - " + buffer_str);
                    /* Message from other clients. */
                    msg.process = atoi(tokens.at(0).c_str());
                    msg.sendclock = atoi(tokens.at(1).c_str());
                    msg.selfclock = clock_count;
                    msg.delivered = false;
                    msg.isseqmsg = false;
                    msg.str = tokens.at(2);

                    /* Print the message if it satisfies condition. */
                    if(is_printable(msg))
                    {
                        log(0, "Delivered - " + buffer_str);
                    }
                    /* Else save the message to the buffer. */
                    else 
                    {
                        insert_message(msg);
                    }
                }
                else if(!sequencer)
                {
                    log(0, "Received - " + buffer_str);
                    /* Message from the sequencer. */
                    msg.process = -1;
                    msg.sendclock = atoi(tokens.at(1).c_str());
                    msg.selfclock = clock_count;
                    msg.isseqmsg = true;
                    msg.str = tokens.at(2) + ":" + tokens.at(3) + ":" + tokens.at(4);

                    /* Print the message if it satisfies condition. */
                    if(is_printable(msg))
                    {
                        log(0, "Delivered - " + msg.str);
                    }
                    /* Else save the message to the buffer. */
                    else {
                        insert_message(msg);
                    }
                }
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
        cerr << "Usage non-ordered: part_two <group_address> <port_number> <groupsize><\n";
		cerr << "Usage ordered (client): part_two <group_address> <port_number> <groupsize> <ordered>\n";
        cerr << "Usage ordered (sequencer): part_two <group_address> <port_number> <groupsize> <ordered> <sequencer>\n";
		exit(1);
	}

    /* Check if the ordering needs to be maintained and designate sequencer. */
    sequencer = false;
    if(argc == 4)
        ordered = false;
    else
        ordered = true;
    if(argc == 6)
        sequencer = true;

    /* Set the clock to a random value with PID as seed. */
	id = getpid();
	clock_count = 0;
    last_recv = 0;
    recv_count = 0;

    /* Setup the log file. */
    std::string filename = "logs/process_" + itos(id) + "log.txt";
    log_file.open(filename.c_str(), ios::out | ios::trunc);
    log(0, "Process started.");

    /* Set up the connection data for the program. */
    struct conn_data *data;
	data = (struct conn_data*) malloc(sizeof(struct conn_data));
    data->host = argv[1];
    data->port = atoi(argv[2]);
    data->groupsize = atoi(argv[3]);

    data->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(data->fd < 0)
    {
        log(1, "Error creating UDP socket. Exiting...");
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