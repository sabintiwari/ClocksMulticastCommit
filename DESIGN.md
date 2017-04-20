## Clocks, Multicast, and Commit | Sabin Raj Tiwari | CMSC 621 Project 2

***
### Design and Results
***

I chose to implement each of the assignment in different programs.

#### Assignment 1

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

#### Assignment 2

The implementation of part two of the project also uses UDP sockets to setup a multicast group. The program can be launched by passing in the multicast group address, the port number, the total number of processes in the group, and (optionally) a flag to make the program use total ordering. If the last parameter, i.e. the total ordering flag is passed then the processes will wait to print the message after ordering else they will print as soon as they receive it. All the processes will start a send and receive thread. The send thread will wait for 3 seconds, then send 50 messages to the multicast group. The receive thread will have a loop that keeps waiting to receive until no messages have been received for 5 seconds. Once 5 seconds have passed, the program will close the socket, the log file, and it will exit.

The ordered version of the program uses a `std::vector` to store the incoming messages. The clocks are added by sorting when they are received. When no more messages are received for the process for longer than 5 seconds, the messages will be delivered to the application. The message that the sender sends is its process id, logical clock count, and a arbitrary message, e.g. "2456:23:MESSAGE_5". When running the ordered version, the script also runs a process with an extra argument that designates it as the sequencer.

This program also outputs to a log file. When running the part_two.sh script, the number of processes that are run will each have a log file created in the logs directory so that it's easier to distinguish the message logs. The logs will have a time stamp, both logical from the sender message as well as the actual time that the receiver process printed the message.

The main issue I encountered was somehow getting unordered outputs when not using the total ordering. Even though there were not ordering algorithms in place, the messages we always being delivered in order when running all the processes on one machine.

##### Assignment 2 Results
`Non-ordered`:
* When using the non-ordered version of the assignment, it was still tough to get the messages to be received out of order because of the fact that testing was done on the same computer. Every non-ordered run of the program resulting in the ordered delivery of the message. The output below shows the running of the non-ordered program with 5 processes.
* Running `part_two.sh` like this `./part_two.sh 224.0.0.1 3003 5 N`, gave the following results:
```bash
[11:02:58:157512][31058][Process] Process started.
[11:02:58:157547][31059][Process] Process started.
[11:02:58:157593][31057][Process] Process started.
[11:02:58:157712][31060][Process] Process started.
[11:02:58:158152][31061][Process] Process started.
[11:03:01:157831][31059][Process] Sending - 31059:1:MESSAGE_1
[11:03:01:157840][31058][Process] Sending - 31058:1:MESSAGE_1
[11:03:01:157865][31057][Process] Sending - 31057:1:MESSAGE_1
[11:03:01:157954][31060][Process] Sending - 31060:1:MESSAGE_1
[11:03:01:158080][31058][Process] Sending - 31058:2:MESSAGE_2
[11:03:01:158080][31059][Process] Sending - 31059:2:MESSAGE_2
[11:03:01:158121][31060][Process] Sending - 31060:2:MESSAGE_2
[11:03:01:158122][31061][Process] Received and Delivered - 31059:1:MESSAGE_1
[11:03:01:158203][31058][Process] Sending - 31058:3:MESSAGE_3
[11:03:01:158234][31061][Process] Received and Delivered - 31058:1:MESSAGE_1
[11:03:01:158218][31059][Process] Sending - 31059:3:MESSAGE_3
[11:03:01:158238][31060][Process] Sending - 31060:3:MESSAGE_3
[11:03:01:158312][31058][Process] Sending - 31058:4:MESSAGE_4
[11:03:01:158321][31061][Process] Received and Delivered - 31057:1:MESSAGE_1
[11:03:01:158368][31060][Process] Sending - 31060:4:MESSAGE_4
[11:03:01:158371][31059][Process] Sending - 31059:4:MESSAGE_4
[11:03:01:158424][31058][Process] Sending - 31058:5:MESSAGE_5
[11:03:01:158420][31061][Process] Sending - 31061:4:MESSAGE_1
[11:03:01:158446][31057][Process] Sending - 31057:2:MESSAGE_2
[11:03:01:158511][31060][Process] Sending - 31060:5:MESSAGE_5
[11:03:01:158511][31059][Process] Sending - 31059:5:MESSAGE_5
[11:03:01:158613][31057][Process] Sending - 31057:3:MESSAGE_3
[11:03:01:158622][31061][Process] Sending - 31061:6:MESSAGE_2
[11:03:01:158727][31057][Process] Sending - 31057:4:MESSAGE_4
[11:03:01:158742][31061][Process] Sending - 31061:7:MESSAGE_3
[11:03:01:158830][31061][Process] Sending - 31061:8:MESSAGE_4
[11:03:01:158853][31057][Process] Sending - 31057:5:MESSAGE_5
[11:03:01:158839][31060][Process] Received and Delivered - 31059:1:MESSAGE_1
[11:03:01:158908][31061][Process] Sending - 31061:9:MESSAGE_5
[11:02:58:157512][31058][Process] Process started.
[11:02:58:157547][31059][Process] Process started.
[11:02:58:157593][31057][Process] Process started.
[11:02:58:157712][31060][Process] Process started.
[11:02:58:158152][31061][Process] Process started.
[11:03:01:157831][31059][Process] Sending - 31059:1:MESSAGE_1
[11:03:01:157840][31058][Process] Sending - 31058:1:MESSAGE_1
[11:03:01:157865][31057][Process] Sending - 31057:1:MESSAGE_1
[11:03:01:157954][31060][Process] Sending - 31060:1:MESSAGE_1
[11:03:01:158080][31058][Process] Sending - 31058:2:MESSAGE_2
[11:03:01:158080][31059][Process] Sending - 31059:2:MESSAGE_2
[11:03:01:158121][31060][Process] Sending - 31060:2:MESSAGE_2
[11:03:01:158122][31061][Process] Received and Delivered - 31059:1:MESSAGE_1
[11:03:01:158203][31058][Process] Sending - 31058:3:MESSAGE_3
[11:03:01:158234][31061][Process] Received and Delivered - 31058:1:MESSAGE_1
[11:03:01:158218][31059][Process] Sending - 31059:3:MESSAGE_3
[11:03:01:158238][31060][Process] Sending - 31060:3:MESSAGE_3
[11:03:01:158312][31058][Process] Sending - 31058:4:MESSAGE_4
[11:03:01:158321][31061][Process] Received and Delivered - 31057:1:MESSAGE_1
[11:03:01:158368][31060][Process] Sending - 31060:4:MESSAGE_4
[11:03:01:158371][31059][Process] Sending - 31059:4:MESSAGE_4
[11:03:01:158424][31058][Process] Sending - 31058:5:MESSAGE_5
[11:03:01:158420][31061][Process] Sending - 31061:4:MESSAGE_1
[11:03:01:158446][31057][Process] Sending - 31057:2:MESSAGE_2
[11:03:01:158511][31060][Process] Sending - 31060:5:MESSAGE_5
[11:03:01:158511][31059][Process] Sending - 31059:5:MESSAGE_5
[11:03:01:158613][31057][Process] Sending - 31057:3:MESSAGE_3
[11:03:01:158622][31061][Process] Sending - 31061:6:MESSAGE_2
[11:03:01:158727][31057][Process] Sending - 31057:4:MESSAGE_4
[11:03:01:158742][31061][Process] Sending - 31061:7:MESSAGE_3
[11:03:01:158830][31061][Process] Sending - 31061:8:MESSAGE_4
[11:03:01:158853][31057][Process] Sending - 31057:5:MESSAGE_5
[11:03:01:158839][31060][Process] Received and Delivered - 31059:1:MESSAGE_1
[11:03:01:158908][31061][Process] Sending - 31061:9:MESSAGE_5
[11:03:01:160248][31058][Process] Received and Delivered - 31061:4:MESSAGE_1
[11:03:01:160274][31057][Process] Received and Delivered - 31059:5:MESSAGE_5
[11:03:01:160274][31059][Process] Received and Delivered - 31061:7:MESSAGE_3
[11:03:01:160299][31060][Process] Received and Delivered - 31057:4:MESSAGE_4
[11:03:01:160338][31058][Process] Received and Delivered - 31060:5:MESSAGE_5
[11:03:01:160349][31059][Process] Received and Delivered - 31057:4:MESSAGE_4
[11:03:01:160355][31057][Process] Received and Delivered - 31061:6:MESSAGE_2
[11:03:01:160363][31060][Process] Received and Delivered - 31061:8:MESSAGE_4
[11:03:01:160421][31058][Process] Received and Delivered - 31059:5:MESSAGE_5
[11:03:01:160425][31059][Process] Received and Delivered - 31061:8:MESSAGE_4
[11:03:01:160426][31057][Process] Received and Delivered - 31061:7:MESSAGE_3
[11:03:01:160445][31060][Process] Received and Delivered - 31057:5:MESSAGE_5
[11:03:01:160510][31059][Process] Received and Delivered - 31057:5:MESSAGE_5
[11:03:01:160516][31057][Process] Received and Delivered - 31061:8:MESSAGE_4
[11:03:01:160506][31058][Process] Received and Delivered - 31057:3:MESSAGE_3
[11:03:01:160523][31060][Process] Received and Delivered - 31061:9:MESSAGE_5
[11:03:01:160591][31059][Process] Received and Delivered - 31061:9:MESSAGE_5
[11:03:01:160597][31057][Process] Received and Delivered - 31061:9:MESSAGE_5
[11:03:01:160606][31058][Process] Received and Delivered - 31061:6:MESSAGE_2
[11:03:01:160724][31058][Process] Received and Delivered - 31061:7:MESSAGE_3
[11:03:01:160807][31058][Process] Received and Delivered - 31057:4:MESSAGE_4
[11:03:01:160891][31058][Process] Received and Delivered - 31061:8:MESSAGE_4
[11:03:01:160976][31058][Process] Received and Delivered - 31057:5:MESSAGE_5
[11:03:01:161060][31058][Process] Received and Delivered - 31061:9:MESSAGE_5
[11:03:06:158573][31058][Process] Exiting...
[11:03:06:158573][31059][Process] Exiting...
[11:03:06:158584][31057][Process] Exiting...
[11:03:06:158584][31060][Process] Exiting...
[11:03:06:159773][31061][Process] Exiting...
```

`Ordered`:
* The ordered version guaranteed the correct delivery of the messages. But as expected, this version took more CPU and Memory usage and was a little slower. When using the ordered version of the assignment, the processes were synchronized with the messages by the sequencer and then they would print the message. This was evident from the results below. The messages that were sent were stored in the buffer until both the message and the sequencer's bersion of the message were received.
* Running `part_two.sh` like this `./part_two.sh 224.0.0.1 3003 3 Y`, gave the following results:
```bash
[11:08:26:335051][31286][Sequencer] Process started.
[11:08:26:335153][31288][Process] Process started.
[11:08:26:335734][31287][Process] Process started.
[11:08:29:335420][31288][Process] Sending - 31288:1:MESSAGE_1
[11:08:29:335753][31287][Process] Received - 31288:1:MESSAGE_1
[11:08:29:335761][31286][Sequencer] Received - 31288:1:MESSAGE_1
[11:08:29:335982][31288][Process] Sending - 31288:2:MESSAGE_2
[11:08:29:336055][31287][Process] Sending - 31287:1:MESSAGE_1
[11:08:29:336176][31287][Process] Received - 31288:2:MESSAGE_2
[11:08:29:336192][31288][Process] Sending - 31288:3:MESSAGE_3
[11:08:29:336194][31286][Sequencer] Received - 31288:2:MESSAGE_2
[11:08:29:336262][31287][Process] Sending - 31287:2:MESSAGE_2
[11:08:29:336262][31288][Process] Received - 31287:1:MESSAGE_1
[11:08:29:336323][31286][Sequencer] Received - 31287:1:MESSAGE_1
[11:08:29:336344][31288][Process] Sending - 31288:4:MESSAGE_4
[11:08:29:336406][31287][Process] Sending - 31287:3:MESSAGE_3
[11:08:29:336428][31286][Sequencer] Received - 31288:3:MESSAGE_3
[11:08:29:336406][31288][Process] Received - 31287:2:MESSAGE_2
[11:08:29:336479][31288][Process] Sending - 31288:5:MESSAGE_5
[11:08:29:336545][31286][Sequencer] Received - 31287:2:MESSAGE_2
[11:08:29:336557][31287][Process] Sending - 31287:4:MESSAGE_4
[11:08:29:336558][31288][Process] Received - 31287:3:MESSAGE_3
[11:08:29:336645][31286][Sequencer] Received - 31288:4:MESSAGE_4
[11:08:29:336638][31287][Process] Received - 31288:3:MESSAGE_3
[11:08:29:336673][31288][Process] Received - 31287:4:MESSAGE_4
[11:08:29:336673][31287][Process] Sending - 31287:5:MESSAGE_5
[11:08:29:336745][31286][Sequencer] Received - 31287:3:MESSAGE_3
[11:08:29:336776][31287][Process] Received - 31288:4:MESSAGE_4
[11:08:29:336811][31288][Process] Received - 31287:5:MESSAGE_5
[11:08:29:336848][31286][Sequencer] Received - 31288:5:MESSAGE_5
[11:08:29:336902][31287][Process] Received - 31288:5:MESSAGE_5
[11:08:29:336956][31286][Sequencer] Received - 31287:4:MESSAGE_4
[11:08:29:337067][31286][Sequencer] Received - 31287:5:MESSAGE_5
[11:08:30:335475][31286][Sequencer] Sending - SEQ:1:31288:1:MESSAGE_1
[11:08:30:335731][31286][Sequencer] Sending - SEQ:2:31288:2:MESSAGE_2
[11:08:30:335726][31287][Process] Received - SEQ:1:31288:1:MESSAGE_1
[11:08:30:335750][31286][Sequencer] Received - SEQ:1:31288:1:MESSAGE_1
[11:08:30:335896][31287][Process] Received - SEQ:2:31288:2:MESSAGE_2
[11:08:30:335942][31286][Sequencer] Received - SEQ:2:31288:2:MESSAGE_2
[11:08:30:335961][31286][Sequencer] Sending - SEQ:3:31287:1:MESSAGE_1
[11:08:30:336145][31286][Sequencer] Received - SEQ:3:31287:1:MESSAGE_1
[11:08:30:336163][31288][Process] Received - SEQ:3:31287:1:MESSAGE_1
[11:08:30:336302][31286][Sequencer] Sending - SEQ:4:31288:3:MESSAGE_3
[11:08:30:336501][31286][Sequencer] Sending - SEQ:5:31287:2:MESSAGE_2
[11:08:30:336481][31287][Process] Received - SEQ:4:31288:3:MESSAGE_3
[11:08:30:336533][31286][Sequencer] Received - SEQ:4:31288:3:MESSAGE_3
[11:08:30:336633][31286][Sequencer] Sending - SEQ:6:31288:4:MESSAGE_4
[11:08:30:336647][31288][Process] Received - SEQ:5:31287:2:MESSAGE_2
[11:08:30:336704][31286][Sequencer] Received - SEQ:5:31287:2:MESSAGE_2
[11:08:30:336766][31286][Sequencer] Sending - SEQ:7:31287:3:MESSAGE_3
[11:08:30:336809][31287][Process] Received - SEQ:6:31288:4:MESSAGE_4
[11:08:30:336828][31286][Sequencer] Received - SEQ:6:31288:4:MESSAGE_4
[11:08:30:336898][31286][Sequencer] Sending - SEQ:8:31288:5:MESSAGE_5
[11:08:30:336920][31288][Process] Received - SEQ:7:31287:3:MESSAGE_3
[11:08:30:336942][31286][Sequencer] Received - SEQ:7:31287:3:MESSAGE_3
[11:08:30:337036][31286][Sequencer] Sending - SEQ:9:31287:4:MESSAGE_4
[11:08:30:337059][31286][Sequencer] Received - SEQ:8:31288:5:MESSAGE_5
[11:08:30:337081][31287][Process] Received - SEQ:8:31288:5:MESSAGE_5
[11:08:30:337212][31286][Sequencer] Sending - SEQ:10:31287:5:MESSAGE_5
[11:08:30:337232][31286][Sequencer] Received - SEQ:9:31287:4:MESSAGE_4
[11:08:30:337370][31286][Sequencer] Received - SEQ:10:31287:5:MESSAGE_5
[11:08:30:337264][31288][Process] Received - SEQ:9:31287:4:MESSAGE_4
[11:08:30:337687][31288][Process] Received - SEQ:10:31287:5:MESSAGE_5
[11:08:35:336034][31288][Process] Delivered - 31287:1:MESSAGE_1
[11:08:35:336082][31288][Process] Delivered - 31287:2:MESSAGE_2
[11:08:35:336095][31288][Process] Delivered - 31287:3:MESSAGE_3
[11:08:35:336107][31288][Process] Delivered - 31287:4:MESSAGE_4
[11:08:35:336116][31288][Process] Delivered - 31287:5:MESSAGE_5
[11:08:35:336124][31288][Process] Exiting...
[11:08:35:336663][31287][Process] Delivered - 31288:1:MESSAGE_1
[11:08:35:336691][31287][Process] Delivered - 31288:2:MESSAGE_2
[11:08:35:336703][31287][Process] Delivered - 31288:3:MESSAGE_3
[11:08:35:336714][31287][Process] Delivered - 31288:4:MESSAGE_4
[11:08:35:336721][31287][Process] Delivered - 31288:5:MESSAGE_5
[11:08:35:336731][31287][Process] Exiting...
[11:08:37:336065][31286][Sequencer] Exiting...
```

#### Assignment 3