# Project 5 - sim5

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
./sim5 sim5.conf
```

#### Load Meta file 5 Times in 100ms interval
```
Line 66: REPEATLOADMETA=5 is how many times the meta file should be loaded
Line 67: sem_loadmeta counts how many times the meta file is loaded so far
Line 400: loadmeta_xtimes() calls loadmeta 5 times in loop wit 100ms interval
Line 282 - 398: loadmeta() loads meta file 1 time
Line 394: incremets sem_loadmeta after finishing loading meta file each time
Line 414: gets value of how many times meta file has been loaded so far
Line 411 - 423: endsimulation only if (1) meta file has been loaded 5 times and (2) READY queue is empty and (3) WAITING queue is empty and (4) no PCB is being transferred from one queue to another.
```

#### PCB READY queue and WAITING queue

1. PCB READY queue and WAITING queue are simulated in separate threads
2. If process is waiting for I/O completion it is moved to WAITING queue
3. After I/O completion it is moved back to READY queue
4. Multiple I/O can be done in parallel
5. e.g. Different PCB can use keyboard, HDD 0, HDD 1, PRIN 0 and PRIN 1 in parallel
6. However, different PCB cannot use HDD 0 at same time (due to mutex lock)
7. Since, it is possible to write to both HDD 0 and HDD 1 at same time, total simulation takes significantly less time (approximately 5.6 seconds rahter than 23.4 seconds) to complete.

```
Line 79 - 82: Global parameters for READY queue
Line 84 - 87: Global parameters for WAITING queue
Line 975 - 1039: Functions for PCB READY queue
Line 1042 - 1106: Functions for PCB WAITING queue

Line 571: simulate READY queue in thread
Line 572: simulate WAITING queue in another thread

Line 452 - 560: function for simulating READY queue
Line 524 - 528: If I/O operation, change PCB state to WAITING and put on WAITING queue
Line 426 - 450: function for simulating WAITING queue
Line 440: get next PCB from WAITING queue
Line 443: call I/O on new non-blocking thread so that multiple I/O can be performed in parallel (device is locked using mutex, so there will be no race condition)
Line 632 - 691: helper function for I/O
Line 682: change pcb state to READY after I/O completion
Line 685: put PCB into READY queue after I/O completion
```

A sample output below explains concurrency of PCB READY queu and WAITING queue
```
0.093000 - Process 2: start hard drive output on HDD 0
0.156000 - Process 3: end processing action
0.156000 - Process 3: start keyboard input
...
0.312000 - OS: preparing process 6
0.312000 - OS: starting process 6
0.312000 - Process 6: start processing action
0.312000 - Process 5: start hard drive output on HDD 1
0.405000 - Process 6: end processing action
0.405000 - OS: preparing process 7
0.405000 - OS: starting process 7
...
0.875000 - Process 15: start processing action
0.952000 - Process 15: end processing action
1.015000 - Process 4: end processing action
1.015000 - Process 2: end hard drive output
```

The above output shows that hard drive output to HDD 0 and HDD 1 are performed concurrently while other processes in READY queue are also processed. This ensures that the READY queue simulation is not delayed by WAITING queue simulation. And many other processes are simulated in READY queue before Process 2: end hard drive output. This ensures that simulation ends in only 5.6 seconds rather than 23.4 seconds.


#### Shortest Time Remaining (STR) Scheduler
```
Line 364: priority of PCB in STR is pcb.remms = total remaining time (ms) of PCB, process id is added to this value so that - if two processes have same reamining time (ms), the one that comes first will have higher priority
```


#### Round Robin (RR) and Interrupt
```
Line 356 - 357: priority of PCB in RR = process id = order of arrival = same as FIFO,
Line 706: rr_timer() generates intterupt (by setting mut_interrupt) in infinite loop
Line 565 - 568: rr_timer() function is run on separate thread only if scheduler is RR, it keeps interrupting the process execution infinetly till end of program
Line 614: function for waiting miliseconds for a running process
Line 621 - 626: if interrupt flag is set, run process for conf.quant = 50 ms. conf.quant is the quantum number in milliseconds. Otherwise run process for total duration of process. Interrupt flag will be set repeatedly in RR so no process will run for more than conf.quant = 50 ms.
Line 628: return how many ms the process was run for
Line 504: get elapsed ms = how long PCB was run for
Line 506: calculate remaining ms based on total and elapsed
Line 510: if remms > 0, it means process was interrupted before completion, in that case make PCB status READY from RUNNING and put it at end of READY queue.
Line 513: since READY queue is a min heap, pcb priority is incremented by 1000 to put it at end of mean heap. 
Line 515: put PCB at end of READY queue
```
