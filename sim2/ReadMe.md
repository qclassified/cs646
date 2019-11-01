# Project 2 - sim2

#### CS 646 Principles of Computer Operating Systems

#### Fall 2019

#### University of Nevada, Reno

#### Farhan Sadique


## Make and run

```
make
./sim2 sim2.conf
```

#### Process Control Block (PCB)
Line 38 - enum pcb_s {START, READY, RUNNING, WAITING, EXIT} decalred
Line 39 - PCB struct declared
Line 621 - PCB pcbarr[10] - holds PCB of 10 processes
Line 669 - pcb state = START
Line 671 - pcb state = READY
Line 674 - pcb state = EXIT
Line 677 - pcb state = RUNNING
Line 683 - pcb state = WAITING
Line 685 - pcb state = RUNNING (after WAITING)

