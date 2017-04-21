/*
	Sabin Raj Tiwari
	CMSC 621
	Project 2
	Assignment 3
*/

/* Include header files. */
#include <arpa/inet.h>
#include <cstring>
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
#define FILENAME "part_three.txt"

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
std::fstream count_file;
int id, logical_clock, used_threads = 0, recv_count = 0;
bool isseq, islocked;

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

/* Check if this process is granted access. */
bool get_access(int id)
{
    int min = -1;
    int minid = -1;
    message_data msg;
    for(int i = 0; i < messages_vector.size(); i++)
    {
        msg = messages_vector.at(i);
        cout << msg.process << ":" << msg.lclock << "\n";
        if(min == -1 || msg.lclock < min)
        {
            min = msg.lclock;
            minid = msg.process;
        }
    }

    return id == minid;
}

/* Removes the item from the message list with the id specified. */
bool remove_by_id(int id)
{   
    int index = -1;
    message_data msg;
    for(int i = 0; i < messages_vector.size(); i++)
    {
        msg = messages_vector.at(i);
        if(id ==  msg.process)
        {
            index = i;
            break;
        }
    }
    if(index > -1)
    {
        messages_vector.erase(messages_vector.begin() + index);
        return true;
    }
    return false;
}

/* Thread that handles each clients request. */
void *seq_thread(void *args)
{
    /* Get the socket data. */
	struct socket_data *data;
	data = (struct socket_data *) args;

    /* Values to store the responses. */
    char buffer[MAXDATASIZE];
    std::string buffer_str, client_str;

    /* Create the string form of the client address. */
	client_str = inet_ntoa(data->address.sin_addr);

    /* Read in the request from the client. */
    memset(&buffer[0], 0, MAXDATASIZE);
    int status = read(data->socket_fd, &buffer, MAXDATASIZE);
    if(status < 0)
    {
        cerr << "Error receiving data from client.\n";
    }

    /* Get the data from the client and store the data. */
    recv_count++;
    buffer[status] = '\0';
    buffer_str = buffer;

    /* Wait for all the processes to send the request. */
    while(recv_count < data->groupsize - 1) { }

    /* Create the message data. */
    std::vector<std::string> tokens = split(buffer_str, ':');
    message_data msg;
    msg.process = atoi(tokens.at(0).c_str());
    msg.lclock = atoi(tokens.at(1).c_str());
    msg.str = tokens.at(2);
    messages_vector.push_back(msg);

    bool end = false;
    int client_id = msg.process;
    int lclock = msg.lclock;
    while(!end)
    {
        /* Check if the file is locked. */
        if(!islocked)
        {
            /* Check if the clock is the lowest for this process. */
            if(get_access(client_id))
            {
                islocked = true;
                cout << "Client [" << client_id << ":" << lclock << "] got access.\n";
                /* Send the write access to the client. */
                status = write(data->socket_fd, &id, sizeof(id));
				if(status < 0)
				{
					cerr << "Error writing data to client.\n";
				}
                
                /* Wait for the release of the lock from the client. */
                int status = read(data->socket_fd, &lclock, sizeof(int));
                if(status < 0)
                {
                    cerr << "Error receiving data from client.\n";
                }

                /* Release lock and end. */
                islocked = false;
                remove_by_id(id);
                end = true;
            }
        }
    }

    /* Close the socket once the request is complete. */
    close(data->socket_fd);
    free(data);
    used_threads--;
}

/* Method that performs the function of the sequencer process that provides access. */
void sequencer(std::string host, int port, int groupsize)
{
    /* Create a stream socket. */
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		cerr < "Error creating socket.\n";
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
        cerr < "Error binding socket. \n";
    }

    /* Wait and listen for a request from a client. */
    cout << "Sequencer started. Listening...\n";
	listen(listen_fd, groupsize - 1);

    /* Setup the variables required for the function. */
    pthread_t threads[groupsize];

    /* Main loop for the server program. */
    while(1)
    {
        /* Create the socket data. */
        struct socket_data *client;
		client = (struct socket_data*) malloc(sizeof(struct socket_data));

        /* Accept a client request. */
		socklen_t client_length = sizeof(client->address);
        client->groupsize = groupsize;
		client->socket_fd = accept(listen_fd, (struct sockaddr *)&(client->address), &client_length);
		if(client->socket_fd < 0)
		{
			cerr << "Error accepting client request.\n";
			return;
		}

        /* Create a thread call the client request function. */
		pthread_create(&threads[used_threads], NULL, seq_thread, (void*) client);
		used_threads++;
    }

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
		cerr << "No host exists with the address: " << host << "\n";
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
		cerr << "Socket is not formed.\n";;
		exit(1);
	}

	/* Try to connect to the server using the address that was created. */
	int status = connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
	if(status < 0)
	{
		/* Show error and exit if the connection fails. */
		cerr << "Failed to connect to the server.\n";
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
        cerr << "Failed to send message to server.\n";
        exit(1);
    }

    /* Wait for the server to send an ok message to read and write to the file. */
    status = read(socket_fd, &code, sizeof(int));
    if(status < 0)
    {
        cerr << "Failed to read message from server.\n";
        exit(1);
    }

    cout << "Client [" << id << ":" << logical_clock << "] got access. Performing read write...\n";

    /* Perform the file write. */
    count_file.open(FILENAME, ios::out | ios::app);
    std::string str_in;
    int num_in;
    if(count_file.is_open())
    {
        if(std::getline(count_file, str_in))
        {
            num_in = atoi(str_in.c_str());
            cout << "Client [" << id << ":" << logical_clock << "] read initial value: " << num_in << "\n";
            num_in++;
            count_file << num_in << "\n";
            cout << "Client [" << id << ":" << logical_clock << "] updated the value: " << num_in << "\n";
        }
    }
    count_file.close();

    /* Verify that the update was made. */
    if(count_file.is_open())
    {
        if(std::getline(count_file, str_in))
        {
            num_in = atoi(str_in.c_str());
            cout << "Client [" << id << ":" << logical_clock << "] read the updated value: " << num_in << "\n";
        }
    }
    count_file.close();

    /* Send the message to the server so that the lock can be released. */
    code++;
    status = write(socket_fd, &code, sizeof(int));
    if(status < 0)
    {
        cerr << "Failed to send message to server.\n";
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