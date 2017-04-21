## Clocks, Multicast, and Commit | Sabin Raj Tiwari | CMSC 621 Project 2

***
### Design and Results
***

I chose to implement each of the assignment in different programs.

#### Assignment 1 `part_one.cpp`

The implementation of part one of the project uses UDP sockets to setup a multicast group. The program can be launched by passing in the multicast group address, the port number, and, (optionally) the total number of processes in the group including the time daemon. If the last parameter, i.e. the group count is passed then the process will make itself the time daemon and will send its clock count to poll all the other clients to send their time offset. In the regular process, the request will be waiting to receive a poll message from the time daemon. When the message is received, the process will check the difference between its clock count and the time daemon's clock count then send a reply with the value. The time daemon, after receiving responses from the other processes, will calculate the average difference of the group, add to its own time and send the updated time as a multicast to all the other processes in the group. The other processes will update their own time after receiving the new clock count then send a reply to the time daemon so the the sockets can be closed.

This assignment helped me learn how UDP sockets can be used to setup a multicast. One thing that was very important when implementing the solution was that there has to be a distinction between how the time daemon and the other processes setup the sockets. The sender process only needed to setup the socket and address for the multicast group address and the process was ready to use the multicasting functionality. While the listener processes had to be bound to the socket and joined to the multicast group in order to receive the messages from the sender.

One of the issues I encountered was making sure the messages were delivered correctly. Initially, I had accidently replaced the time daemon's group address variable with the receiving address of one of the proceses whenever `recvfrom` was called. This made the time daemon only reply with the updated clock to the last process that sent its time offset. After storing the sender's address in a different variable, I was able to successfully send the updated time to all the other processes and correctly synchronize the time for the group.

Another issue I had was deciding how to handle which process would become the time daemon. Since leader election was not part of the requirements, I decided that the leader will be elected on process execution. This makes it important to run the other processes beforehand and then running the time daemon to poll to start the clock synchronization process. The bash script `part_one.sh` makes sure the processes are started in a way that the time daemon will properly poll and start the synchronization.

##### Assignment 1 Results
Running `part_one.sh` like this `./part_one.sh 224.0.0.1 3003 5`, gave the following results:
```bash
[Process 30932] Started. Current clock: 86
[Process 30934] Started. Current clock: 3
[Process 30936] Started. Current clock: 5
[Process 30938] Started. Current clock: 42
[Process 30940][Time Daemon] Started. Current clock: 3
[Process 30934] Sending: 0
[Process 30938] Sending: 39
[Process 30936] Sending: 2
[Process 30932] Sending: 83
[Process 30940][Time Daemon] Clock updated: 27
[Process 30938] Clock updated: 27
[Process 30936] Clock updated: 27
[Process 30934] Clock updated: 27
[Process 30932] Clock updated: 27
```

#### Assignment 2 `part_two.cpp`

The implementation of part two of the project also uses UDP sockets to setup a multicast group. The program can be launched by passing in the multicast group address, the port number, the total number of processes in the group, and (optionally) a flag to make the program use total ordering. If the last parameter, i.e. the total ordering flag is passed then the processes will wait to print the message after ordering else they will print as soon as they receive it. All the processes will start a send and receive thread. The send thread will wait for 3 seconds, then send 50 messages to the multicast group. The receive thread will have a loop that keeps waiting to receive until no messages have been received for 5 seconds. Once 5 seconds have passed, the program will close the socket, the log file, and it will exit.

The ordered version of the program uses a `std::vector` to store the incoming messages. The clocks are added by sorting when they are received. When no more messages are received for the process for longer than 5 seconds, the messages will be delivered to the application. The message that the sender sends is its process id, logical clock count, and a arbitrary message, e.g. "2456:23:MESSAGE_5". When running the ordered version, the script also runs a process with an extra argument that designates it as the sequencer.

This program also outputs to a log file. When running the part_two.sh script, the number of processes that are run will each have a log file created in the logs directory so that it's easier to distinguish the message logs. The logs will have a time stamp, both logical from the sender message as well as the actual time that the receiver process printed the message.

The main issue I encountered was somehow getting unordered outputs when not using the total ordering. Even though there were not ordering algorithms in place, the messages we always being delivered in order when running all the processes on one machine.

##### Assignment 2 Results
`Non-ordered`:
* When using the non-ordered version of the assignment, it was still tough to get the messages to be received out of order because of the fact that testing was done on the same computer. I even started one client on a different machine on the same network but the ordering of the messages were preserved. Every non-ordered run of the program resulted in the ordered delivery of the message. It was impossible to make the network itself deliver messages out of order. So even though no ordering was implemented, the delivery (printing) of the message was always in order.
* The following is the output file from one of the processes the non-ordered program when running the bash script like so: `./part_two.sh 224.0.0.1 3003 3 N`. The process received messages from the other two processes but delivered every single one of them in order even though there was no sequencer algorithm in use. The messages have `sender_address`:`sender_process_id`:`sender_clock`:`sender_message_number` as the output.
```bash
[01:23:40:423514][11910][Process] Process started.
[01:23:43:423694][11910][Process] Received and Delivered - 0.0.0.0:11908:1:MESSAGE_1
[01:23:43:423818][11910][Process] Received and Delivered - 0.0.0.0:11909:1:MESSAGE_1
[01:23:43:423896][11910][Process] Received and Delivered - 0.0.0.0:11908:2:MESSAGE_2
[01:23:43:423981][11910][Process] Received and Delivered - 0.0.0.0:11909:2:MESSAGE_2
[01:23:43:424061][11910][Process] Received and Delivered - 0.0.0.0:11908:4:MESSAGE_3
[01:23:43:424117][11910][Process] Sending - 0.0.0.0:11910:6:MESSAGE_1
[01:23:43:424117][11910][Process] Sending - 0.0.0.0:11910:6:MESSAGE_1
[01:23:43:424150][11910][Process] Received and Delivered - 0.0.0.0:11909:3:MESSAGE_3
[01:23:43:424367][11910][Process] Sending - 0.0.0.0:11910:8:MESSAGE_2
[01:23:43:424513][11910][Process] Sending - 0.0.0.0:11910:9:MESSAGE_3
[01:23:48:424629][11910][Process] Exiting...

```

`Ordered`:
* The ordered version guaranteed the correct delivery of the messages. But as expected, this version took more CPU and Memory usage and was a little slower. When using the ordered version of the assignment, the processes were synchronized with the messages by the sequencer and then they would print the message. This was evident from the results below. The messages that were sent were stored in the buffer until both the message and the sequencer's version of the message were received and the process sequence number was the global sequencer number minus 1. 
* The following is the output from all the processes in the ordered program when running the bash script like so: `./part_two.sh 224.0.0.1 3003 3 Y`. The processes were coded to send 3 messages each. The submitted assignment sends 5 messages each. The messages have `sender_address`:`sender_process_id`:`sender_clock`:`sender_message_number` as the output.
```bash
[01:19:59:686962][11518][Sequencer] Process started.
[01:19:59:687106][11519][Process] Process started.
[01:19:59:687416][11520][Process] Process started.
[01:20:02:687891][11520][Process] Sending - 0.0.0.0:11520:1:MESSAGE_1
[01:20:02:687903][11519][Process] Sending - 0.0.0.0:11519:1:MESSAGE_1
[01:20:02:688113][11520][Process] Sending - 0.0.0.0:11520:2:MESSAGE_2
[01:20:02:688306][11519][Process] Sending - 0.0.0.0:11519:2:MESSAGE_2
[01:20:02:688319][11520][Process] Sending - 0.0.0.0:11520:3:MESSAGE_3
[01:20:02:688431][11519][Process] Sending - 0.0.0.0:11519:3:MESSAGE_3
[01:20:05:687990][11518][Sequencer] Sending - SEQ:11518:1:0.0.0.0:11519:1:MESSAGE_1
[01:20:05:688264][11520][Process] Delivered - 0.0.0.0:11519:1:MESSAGE_1
[01:20:05:688402][11518][Sequencer] Sending - SEQ:11518:2:0.0.0.0:11520:1:MESSAGE_1
[01:20:05:688568][11518][Sequencer] Sending - SEQ:11518:3:0.0.0.0:11520:2:MESSAGE_2
[01:20:05:688624][11519][Process] Delivered - 0.0.0.0:11520:1:MESSAGE_1
[01:20:05:688721][11518][Sequencer] Sending - SEQ:11518:4:0.0.0.0:11519:2:MESSAGE_2
[01:20:05:688871][11519][Process] Delivered - 0.0.0.0:11520:2:MESSAGE_2
[01:20:05:688884][11518][Sequencer] Sending - SEQ:11518:5:0.0.0.0:11520:3:MESSAGE_3
[01:20:05:688943][11520][Process] Delivered - 0.0.0.0:11519:2:MESSAGE_2
[01:20:05:689029][11518][Sequencer] Sending - SEQ:11518:6:0.0.0.0:11519:3:MESSAGE_3
[01:20:05:689099][11519][Process] Delivered - 0.0.0.0:11520:3:MESSAGE_3
[01:20:05:689220][11520][Process] Delivered - 0.0.0.0:11519:3:MESSAGE_3
[01:20:10:688668][11520][Process] Exiting...
[01:20:10:689291][11519][Process] Exiting...
[01:20:13:689316][11518][Sequencer] Exiting...
```

#### Assignment 3 `part_three.cpp`
The design of the third assignment is also in a different program. The program uses `pthread` library to handle access to the file. First of all, when the sever starts, it waits for a connection coming from the client then starts a thread to handle the file lock for the specific client. The thread uses `pthread_mutex` and `pthread_cond` to lock a variable that will be used to give access to the thread. The thread will notify the client with an `OK` to read and write to the file. Then the client that receives the `OK` will read the file, update the value, write to the file, and send a message to the server to signal that it has finished. Then the server will give access to the next thread that is waiting.
* The following is the output from all the clients and the server when running like so: `./part_three.sh localhost 3005 5`.

```bash
[02:29:31:165478][25510][Server] Started. Listening...
[02:29:31:267757][25510][Server] Received request from a client: 127.0.0.1
[02:29:31:268157][25513][Client] Waiting for server to send access.
[02:29:31:369905][25516][Client] Waiting for server to send access.
[02:29:31:369954][25510][Server] Received request from a client: 127.0.0.1
[02:29:31:470866][25519][Client] Waiting for server to send access.
[02:29:31:470866][25510][Server] Received request from a client: 127.0.0.1
[02:29:31:572210][25522][Client] Waiting for server to send access.
[02:29:31:572315][25510][Server] Received request from a client: 127.0.0.1
[02:29:31:572386][25516][Client] Got access. Performing read/write...
[02:29:31:572436][25516][Client] Reading initial value: 0
[02:29:31:572570][25516][Client] Updating the value: 1
[02:29:31:572648][25516][Client] Reading the updated value: 1
[02:29:31:572738][25513][Client] Got access. Performing read/write...
[02:29:31:572787][25513][Client] Reading initial value: 1
[02:29:31:572855][25513][Client] Updating the value: 2
[02:29:31:572900][25513][Client] Reading the updated value: 2
[02:29:31:573061][25519][Client] Got access. Performing read/write...
[02:29:31:573101][25519][Client] Reading initial value: 2
[02:29:31:573157][25519][Client] Updating the value: 3
[02:29:31:573240][25519][Client] Reading the updated value: 3
[02:29:31:573443][25522][Client] Got access. Performing read/write...
[02:29:31:573489][25522][Client] Reading initial value: 3
[02:29:31:573553][25522][Client] Updating the value: 4
[02:29:31:573598][25522][Client] Reading the updated value: 4
[02:29:36:165985][25510][Server] Exiting...
```