/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 3
*/

/* Include header files. */
#include <arpa/inet.h>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

/* Define the global constants. */
#define MAXDATASIZE 1024
#define MAXUPDATES 5
#define FILENAME "./part_three.txt"

/* Import namespaces. */
using namespace std;

/* Structures for the program. */
struct socket_data
{
	int socket_fd, groupsize;
	struct sockaddr_in address;
};
struct message_data
{
    int process, lclock;
    std::string str;
};

/* Global Variables */
std::vector<struct message_data> messages_vector;
int id, logical_clock, used_threads = 0, recv_count = 0, last_recv = 0;
bool isseq, islocked;
pthread_mutex_t file_lock;
pthread_cond_t file_cond;

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
void log(std::string message)
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
    if(isseq)
    {
        stamp = stamp.append("[Server]");
    }
    else
    {
        stamp = stamp.append("[Client]");
    }
        
    /* Log to cout if its a regular message. */
    cout << stamp << " " << message << "\n";     
}

/* Function that handles the timeout for the accept calls for 5 seconds. */
void *timer(void *args)
{
    if(isseq)
    {
        /* Get the connection data. */
        int data = *((int *) args);

        int end = 0;
        while(end == 0)
        {
            sleep(1);
            last_recv++;
            if((last_recv >= 5))
            {
                end = 1;
            }
        }

        /* Exit the program when the timeout is reached. */
        log("Exiting...");
        close(data);
        exit(0);
    }
    
}

/* Thread that handles each clients request. */
void *seq_thread(void *args)
{
    /* Get the socket data. */
	socket_data data = *((socket_data*)args);

    /* Values to store the responses. */
    char buffer[MAXDATASIZE];
    std::string buffer_str, client_str;

    /* Create the string form of the client address. */
	client_str = inet_ntoa(data.address.sin_addr);

    /* Read in the request from the client. */
    memset(&buffer[0], 0, MAXDATASIZE);
    int status = read(data.socket_fd, &buffer, MAXDATASIZE);
    if(status < 0)
    {
        log("Error receiving data from client.");
    }

    /* Get the data from the client and store the data. */
    recv_count++;
    buffer[status] = '\0';
    buffer_str = buffer;

    /* Wait for all the processes to send the request. */
    while(recv_count < data.groupsize - 1) { }

    /* Create the mutex lock point. */
    pthread_mutex_lock(&file_lock);

    if(islocked)
    {
        pthread_cond_wait(&file_cond, &file_lock);
    }

    islocked = true;
    
    /* Send the write access to the client. */
    status = write(data.socket_fd, &id, sizeof(id));
    if(status < 0)
    {
        log("Error writing data to client.");
    }
    
    /* Wait for the release of the lock from the client. */
    status = read(data.socket_fd, &status, sizeof(int));
    if(status < 0)
    {
        log("Error receiving data from client.");
    }

    /* Unlock the record after writing */
    islocked = false;
    //remove_by_id(id);
    pthread_cond_signal(&file_cond);
    pthread_mutex_unlock(&file_lock);

    /* Close the socket once the request is complete. */
    close(data.socket_fd);
    used_threads--;
}

/* Method that performs the function of the sequencer process that provides access. */
void sequencer(std::string host, int port, int groupsize)
{
    /* Create a stream socket. */
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		log("Error creating socket.");
		exit(0);
	}

    /* Setup the port and the server address. */
    struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

    /* Bind the server to the listener. */
    int status = bind(listen_fd, (struct sockaddr *)&address, sizeof(address));
    if(status < 0)
    {
        log("Error binding socket.");
        exit(0);
    }

    /* Wait and listen for a request from a client. */
    log("Started. Listening...");
	listen(listen_fd, groupsize - 1);

    /* Setup the variables required for the function. */
    pthread_t timeout;
    int *arg = (int *)malloc(sizeof(arg));
    pthread_create(&timeout, 0, timer, arg);
    pthread_t threads[groupsize];
    pthread_cond_init(&file_cond, NULL);
	pthread_mutex_init(&file_lock, NULL);
    std::string client_addr_str;

    /* Main loop for the server program. */
    while(1)
    {
        /* Create the socket data. */
        socket_data client;

        /* Accept a client request. */
		socklen_t client_length = sizeof(client.address);
        client.groupsize = groupsize;
		client.socket_fd = accept(listen_fd, (struct sockaddr *)&(client.address), &client_length);
		if(client.socket_fd < 0)
		{
            log("Error accepting client request.");
			return;
		}

        /* Create a thread call the client request function. */
		pthread_create(&threads[used_threads], NULL, seq_thread, (void*) &client);
		used_threads++;

        /* Write a message. */
        client_addr_str = inet_ntoa(client.address.sin_addr);
        log("Received request from a client: " + client_addr_str);
    }

    pthread_join(timeout, NULL);
    close(listen_fd);
}

/* Method that performs the function of the client that need access to file. */
void client(std::string host, int port)
{	
    /* Setup the connection information to the server. */
	struct hostent *server;
    server = gethostbyname(host.c_str());
	if(server == NULL)
	{
		/* Show error if the server does not exist. */
		log("No host exists with the address: " + host);
		exit(0);
	}

    /* Get the address to the server. */
    struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(port);

    /* Setup the socket. */
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0)
	{
		/* Show error if the socket descriptor fails. */
		log("Socket is not formed.");
		exit(1);
	}

	/* Try to connect to the server using the address that was created. */
	int status = connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
	if(status < 0)
	{
		/* Show error and exit if the connection fails. */
		log("Failed to connect to the server.");
		exit(1);
	}

    /* Update the file a limited number of times. */
    std::string buffer_str;
    int code;

    /* Send a message to get the lock. */
    buffer_str = itos(id) + ":" + itos(logical_clock) + ":REQUEST";
    status = write(socket_fd, buffer_str.c_str(), buffer_str.size());
    if(status < 0)
    {
        log("Failed to send message to server.");
        exit(1);
    }

    /* Client waiting. */
    log("Waiting for server to send access.");

    /* Wait for the server to send an ok message to read and write to the file. */
    status = read(socket_fd, &code, sizeof(int));
    if(status < 0)
    {
        log("Failed to read message from server.");
        exit(1);
    }

    log("Got access. Performing read/write...");

    /* Perform the file read initially. */
    std::ifstream infile(FILENAME, ios::in);
    std::string str_in;
    int num_in;

    if(infile.is_open())
    {
        if(!infile.eof())
        {
            infile >> str_in;
            num_in = atoi(str_in.c_str());
            log("Reading initial value: " + itos(num_in));
        }
    }
    infile.close();

    /* Perform the file write. */
    num_in++;
    std::ofstream outfile(FILENAME, ios::out);
    if(outfile.is_open())
    {
        log("Updating the value: " + itos(num_in));
        outfile << num_in << endl;
    }
    outfile.close();


    /* Verify that the update was made. */
    infile.open(FILENAME, ios::out | ios::app);
    if(infile.is_open())
    {
        if(!infile.eof())
        {
            infile >> str_in;
            num_in = atoi(str_in.c_str());
            log("Reading the updated value: " + itos(num_in));
        }
    }
    infile.close();

    /* Send the message to the server so that the lock can be released. */
    code++;
    status = write(socket_fd, &code, sizeof(int));
    if(status < 0)
    {
        log("Failed to send message to server.");
        exit(1);
    }

    /* Close the connection. */
    close(socket_fd);
}

/* Main method to execute the program logic. */
int main(int argc, char **argv)
{
    if(argc < 3)
	{
		/* Show error if the correct number of arguments were not passed. */
		cerr << "Usage ordered (client): part_two <group_address> <port_number> \n";
        cerr << "Usage ordered (sequencer): part_two <group_address> <port_number> <groupsize>\n";
		exit(1);
	}

    /* Check if the program is supposed to be a sequencer. */
    isseq = false;
    if(argc == 5)
        isseq = true;

    /* Set the clock to a random value with PID as seed. */
	id = getpid();
	srand(id);
	logical_clock = rand() % 100;

    /* Start the client function if its not a sequencer. */
    if(!isseq)
    {
        client(argv[1], atoi(argv[2]));
    }
    else
    {
        sequencer(argv[1], atoi(argv[2]), atoi(argv[3]));
    }

	/* Verify that the address and port are passed. */
	std::string host = argv[1];
	std::string port_str = argv[2];

    return 0;
}