## Clocks, Multicast, and Commit | Sabin Raj Tiwari | CMSC 621 Project 2

***
#### Design
***

I chose to implement each of the assignment in different programs.

### Assignment 1
The implementation of part one of the project uses UDP sockets to setup a multicast group. The program can be launched by passing in the multicast group address, the port number, and, (optionally) the total number of processes in the group including the time daemon. If the last parameter, i.e. the group count is passed then the process will make itself the time daemon and will send its clock count to poll all the other clients to send their time offset. In the regular process, the request will be waiting to receive a poll message from the time daemon. When the message is received, the process will check the difference between its clock count and the time daemon's clock count then send a reply with the value. The time daemon, after receiving responses from the other processes, will calculate the average difference of the group, add to its own time and send the updated time as a multicast to all the other processes in the group. The other processes will update their own time after receiving the new clock count then send a reply to the time daemon so the the sockets can be closed.

This assignment helped me learn how UDP sockets can be used to setup a multicast. One thing that was very important when implementing the solution was that there has to be a distinction between how the time daemon and the other processes setup the sockets. The sender process only needed to setup the socket and address for the multicast group address and the process was ready to use the multicasting functionality. While the listener processes had to be bound to the socket and joined to the multicast group in order to receive the messages from the sender.

One of the issues I encountered was making sure the messages were delivered correctly. Initially, I had accidently replaced the time daemon's group address variable with the receiving address of one of the proceses whenever `recvfrom` was called. This made the time daemon only reply with the updated clock to the last process that sent its time offset. After storing the sender's address in a different variable, I was able to successfully send the updated time to all the other processes and correctly synchronize the time for the group.

Another issue I had was deciding how to handle which process would become the time daemon. Since leader election was not part of the requirements, I decided that the leader will be elected on process execution. This makes it important to run the other processes beforehand and then running the time daemon to poll to start the clock synchronization process. The bash script `part_one.sh` makes sure the processes are started in a way that the time daemon will properly poll and start the synchronization.

### Assignment 2

### Assignment 3