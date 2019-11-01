# Project 3 - sim3

#### CS 646 Principles of Computer Operating Systems

#### Fall 2019

#### University of Nevada, Reno

#### Farhan Sadique


## Make and run

```
make
./sim3 sim3.conf
```

#### Memory Allocation

Line 294: Get current memory block head and increment the tail by one for future allocation
Line 295-296: If memoryblock tail exceeds total memory, circle back to beginning (0)
Lin3 299: Output memory location in console or monitor as 8 digit hex number

#### Semaphores and Mutex

Global variables for semaphore and mutex in Line 53-59

```
Line 53: unsigned int printsem = 1;                        // printer semaphore
Line 54: unsigned int hddsem = 1;                          // HDD semaphore
Line 55: unsigned int curprint = 0;                        // current printer in use
Line 56: unsigned int curhdd = 0;                          // current HDD in use
                                                   
Line 57: unsigned int monmut = 1;                          // monitor mutex
Line 58: unsigned int mousemut = 1;                        // mouse mutex
Line 59: unsigned int keybmut = 1;                         // keyboard mutex

Line 358: Function wait_t is entry point for all I/O operations

Line 370-383: Get current lock status of keyboard, mouse, monitor, hard drive and printer. 
There can be only 1 of each for keyboard, mouse, printer. There can be multiple hdd and printer as per config.

Line 388: While loop waits for particular I/O resource for 1000ms or 1second.
If not available in 1second, throw error and exit program.

Line 391: If resource available lock it for wait time

Line 404: Increment current printer or hdd number by 1 to use next one next time
Line 405: If current printer or hdd number greater than max circle back to first one

Line 412: unlock previously locked resource
```