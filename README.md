## Clocks, Multicast, and Commit | Sabin Raj Tiwari | CMSC 621 Project 2

***
#### Make
***
If there are already compiled files in the `src` directory, first run:

```bash
make clean
```

Then, to build the program, run:

```bash
make compile
```

This will create executable files: `part_one`, `part_two`, and `part_three`

***
#### Run
***
The three executables in the program can be run using their respective bash scripts.

###### Assignment 1 (parameters: `multicast_group_address`, `port_number`, `group_size`):
The `multicast_group_address` has to be a valid multicast address within range `224.0.0.0` to `239.255.255.255`.
```bash
part_one.sh 224.0.0.1 3000 5
```
The logs from the program will be shown in the terminal with the required outputs.

###### Assignment 2 (parameters: `multicast_group_address`, `port_number`, `group_size`, `ordering_flag`):
The `multicast_group_address` has to be a valid multicast address within range `224.0.0.0` to `239.255.255.255`. The `ordering_flag` parameter should be a `Y` for the ordered program and `N` for the non-ordered.
* Ordered
```bash
part_two.sh 224.0.0.1 3000 5 Y
```
* Non-ordered
```bash
part_two.sh 224.0.0.1 3000 5 N
```
If `ordering_flag` is set to `Y`, then the script will run a sequencer and `group_size - 1` number client processes. If `ordering_flag` is set to `N`, then the script will run `group_size` number of client processes that multicast to each other. The logs from the program will be shown for each process in the terminal as well as the `logs` directory.

###### Assignment 3 (parameters: `multicast_group_address`, `port_number`, `group_size`):
```bash
part_three.sh 224.0.0.1 3000 5
```
The script runs `groupsize - 1` client processes that will try to read and write to the file. It will also execute a process that will give read and write access to the other processes.