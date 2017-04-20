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

###### Assignment 2 (parameters: `multicast_group_address`, `port_number`, `group_size`, `ordering_flag`(optional)):
The `multicast_group_address` has to be a valid multicast address within range `224.0.0.0` to `239.255.255.255`. The `ordering_flag` parameter can contain any data because as long as its passed the ordering program will be executed.
```bash
part_two.sh 224.0.0.1 3000 5 Y
```
The logs from the program will be shown for each process in the terminal as well as the `logs` directory.

###### Assignment 3 ():
```bash
part_three.sh
```

