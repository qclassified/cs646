# Project 4 - sim4

#### CS 646 Principles of Computer Operating Systems

#### Fall 2019

#### University of Nevada, Reno

#### Farhan Sadique


## Make and run

#### Make
```
make
```

#### Run
```
./sim4 sim4.conf
```

#### Memory Allocation

Implemented using FIFO queue (int array) of memory block heads. 
memqget() gets next memmory block head, memqput(int) puts freed up memory at end of queue.

```
Line 419 - Line 477: FIFO queue of memory blocks and memory swapping Functions
Line 75-77: Global array for memory queue
Line 224: For loop puts head of each memory block into memory queue
Line 305: memory allocate task gets next memory block in front of memory queue
Line 309: allocate memory block to PCB 
Line 440-447: if memory queue is not empty, return memory block on top of memory queue
Line 458-465: if memory queue is empty, swap/free up memory allocated to idle processes in ready queue
Line 466: error -- not enough memory even after swapping all memory allocated to all idle processes
Line 279: free up/ recapture memory allocated to exiting process
Line 450: function used to recapture memory from PCB
```

#### Scheduler

Implemented using min heap / priority queue. 
Global ready queue is a min heap where key is priority.
So, process with lower priority number (higher priority) will be executed first.

```
Line 479 - 545: Ready queue = priority queue = min heap functions
Line 70-72: Initialize global priority queue as array
Line 385 - 386: 
    FIFO -- pcb.priority is assigned to process id, 
    process id is incremented in order of arrival. 
    Since PCBs are pushed to the min heap, 
    the pcb with lowest process id will be on top of heap
    ensuring FIFO order
Line 387 - 388:
    PS -- Priority Schedule
    pcb.priority = 100-pcb.ionum, pcb.ionum = total number of I/O operations
    Since pcb is pushed to min heap, the one with higher ionum will be on top
    However, heapsort is not stable, means mean heap does not maintain FIFO 
    order for processes with equal priority. So, 
    pcb.priority = 1000*(100-pcb.ionum) + pcb.id,  process id is added to priority
    this ensures process 2 will have lower priority number than process 3
    even if they both have ionum==5. 1000 factor ensures pcb.id does not
    offset contribution of pcb.ionum keeping ionum the dominant factor
Line 389 - 390:
    SJF -- Shortest Job First
    pcb.priority = pcb.inum
    pcb.inum = total number of  instructions /tasks for process
    the pcb with lowest inum will be pushed to top of min heap
Line 391 - 392:
    STF -- Shortest Time First
    pcb.priority = burst time (in millisecond)
    The one with lowest burst time will be pushed to top of min heap
Line 396: Push PCB to ready queue/ priority queue/ min heap
Line 249 - 253: Get next PCB from ready queue until it is empty
Line 353: S{begin} has priority 0, so will always be on top of priority queue
Line 362: 
    S{finish} has prioirty INT_MAX (maximum possible priority) 
    so S{finish} will always be at end of priority queue
```