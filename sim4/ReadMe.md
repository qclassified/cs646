# Project 4 - sim4

#### CS 646 Principles of Computer Operating Systems

#### University of Nevada, Reno

#### Farhan Sadique


## Make and run

```
make
./sim4 sim4.conf
```

#### Memory Allocation

Implemented using min heap of memory block heads. 
Process gets next available memory block from beginning.

```
Line 422 - Line 516: Memory heap (min heap) and Memory swapping functions
Line 75-76: Global array for memory heap
Line 227: For loop puts head of each memory block into memory heap
Line 308: memory allocate task gets next memory block on top of memory heap
Line 312: allocate memory block to PCB 
Line 469: if memory heap is not empty, return memory block on top of memory heap
Line 496: if memory heap is empty, swap/free up memory allocated to idle processes in ready queue
Line 504: error -- not enough memory even after swapping all memory allocated to all idle processes
Line 282: free up/ recapture memory allocated to exiting process
Line 488: function used to recapture memory from PCB
```

#### Scheduler

Implemented using min heap. 
Global ready queue is a min heap where key is priority.
So, process with lower priority number (higher priority) will be executed first.

```
Line 517 - 582: Ready queue = priority queue = min heap functions
Line 70-72: Initialize global priority queue as array
Line 388: FIFO -- pcb.priority is assigned to process id, 
    process id is incremented in order of arrival. 
    Since PCBs are pushed to the min heap, 
    the pcb with lowest priority will be on top of heap
```