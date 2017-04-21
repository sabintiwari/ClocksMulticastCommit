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
    bool isseqmsg, isselfsent;
    std::string addr, str;
};
struct buffer_data
{
    char bfr[MAX_DATASIZE];
};

/* Global Variables. */
int id, clock_count, last_recv, recv_count, used_threads = 0;
bool ordered, sequencer;
std::vector<struct message> message_vector;
std::ofstream log_file;
std::string addr_str;

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

/* Checks if the messages can be printed. */
void print_messages()
{
    last_recv = 0;
    if(message_vector.empty()) return;

    /* Iterate through the messages and check if there exists a sequencer or a process message.*/
    message a, b;
    std::string msg;
    std::vector<int> indeces;

    try 
    {
        for(unsigned i = 0; i < message_vector.size(); ++i) {
            a = message_vector.at(i);
            
            /* If the message is a sequence message or the message has already been delivered. */
            if(a.isseqmsg) continue;

            /* Iterate through all the messages again. */
            for(unsigned j = 0; j < message_vector.size(); ++j)
            {
                b = message_vector.at(j);
                if(b.isseqmsg)
                {
                    /* If the sendclock and recv_count are in sync, deliver the message. */
                    msg = a.addr + ":" + itos(a.process) + ":" + itos(a.sendclock) + ":" + a.str;
                    if(b.sendclock == recv_count + 1 && msg == b.str)
                    {
                        if(!a.isselfsent)
                        {
                            log(0, "Delivered - " + b.str);
                        }
                        recv_count++;
                        indeces.push_back(i);
                        indeces.push_back(j);
                    }
                }
            }
        }
    }
    catch(int e)
    {
        //do nothing
    }
        
    for(int i = 0; i < indeces.size(); i++)
        message_vector.erase(message_vector.begin() + indeces.at(i));
}

/* Function that inserts the message at the next possible index based on sort. */
bool insert_message(message a) 
{
    message_vector.push_back(a);
    return true;
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
        if((!sequencer && last_recv >= 5) || (sequencer && last_recv >= 8))
        {
            end = 1;
        }
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
        sleep(3);
        /* If the process is a sequencer, send the sequencing message. */
        while(1)
        {
            if(message_vector.size() > 0)
            {
                msg = message_vector.at(0);
                clock_count++;
                buffer_str = "SEQ:" + itos(id) + ":" + itos(clock_count) + ":" + msg.addr + ":" + itos(msg.process) + ":" + itos(msg.sendclock) + ":" + msg.str;
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
            buffer_str = addr_str + ":" + itos(id) + ":" + itos(clock_count) + ":" + "MESSAGE_" + itos(i + 1);
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

void save_message(std::string buffer_str)
{
    /* Filter the messages from itself. */
    bool isselfsent = buffer_str.find(":" + itos(id) + ":") != std::string::npos;
    if(!isselfsent || !sequencer)
    {
        if(!ordered)
        {
            /* Print the message received from the multicast. */
            if(!isselfsent)
            {
                clock_count++;
                log(0, "Received and Delivered - " + buffer_str);
            }   
        }
        else
        {
            /* Store the message to send later. */
            message msg;
            msg.isselfsent = isselfsent;
            msg.selfclock = clock_count;

            std::vector<std::string> tokens = split(buffer_str, ':');
            if(tokens.size() == 4)
            {
                //if(!isselfsent) log(0, "Received - " + buffer_str);
                /* Message from other clients. */
                msg.addr = tokens.at(0);
                msg.process = atoi(tokens.at(1).c_str());
                msg.sendclock = atoi(tokens.at(2).c_str());
                msg.isseqmsg = false;
                msg.str = tokens.at(3);
            }
            else if(!sequencer)
            {
                //if(!isselfsent) log(0, "Received - " + buffer_str);
                /* Message from the sequencer. */
                msg.addr = "SEQ";
                msg.process = atoi(tokens.at(1).c_str());
                msg.sendclock = atoi(tokens.at(2).c_str());
                msg.isseqmsg = true;
                msg.str = tokens.at(3) + ":" + tokens.at(4) + ":" + tokens.at(5) + ":" + tokens.at(6);
            }

            /* Call the check for print to refresh message status. */
            if(msg.str != "" && (id != msg.process || !sequencer)) {
                insert_message(msg);
                //log(0, "ADD MESSAGE" + buffer_str);
            }
        }
    }

    print_messages();
    used_threads--;
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

    addr_str = inet_ntoa(address.sin_addr);

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
    std::vector<buffer_data> buffers(data->groupsize);
    std::vector<std::string> buffers_strs(data->groupsize);
    pthread_t threads[data->groupsize];
    int i = 0;
    
    while(1)
    {
        /* Wait to receive message from other processes. */
        if(i >= 5) i = 0;
        memset(&(buffers[i].bfr), 0, MAX_DATASIZE);
        status = recvfrom(data->fd, &(buffers[i].bfr), MAX_DATASIZE, 0, (struct sockaddr *)&address, &addr_length);
        if(status < 0)
        {
            log(1, "Exiting...");
            exit(1);
        }

        /* Launch the thread for the received message. */
        buffers[i].bfr[status] = '\0';
        buffers_strs[i] = buffers[i].bfr;
        save_message(buffers_strs[i]);
        //pthread_create(&threads[used_threads], NULL, save_message, (void *) buffers_strs[i].c_str());
        used_threads++;
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

    /* Join the three threads. */
    pthread_join(send, NULL);
    pthread_join(receive, NULL);  
    pthread_join(timeout, NULL);  

    close(data->fd);
    log_file.close();
    return 0;
}