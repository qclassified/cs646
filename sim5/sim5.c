/**
* sim5.c
*
* Project 5
*
* CS 646 Fall 2019
* 
* To compile, run makefile by entering "make"
*
* To run ./sim5 sim5.conf
*
* Farhan Sadique
*/
#include <limits.h>                                        // used only for MAX_INT
#include <stdarg.h>                                        // va_start(), va_end()
#include <stdio.h>                                         // vprintf(), FILE(), fopen() ...
#include <stdlib.h>                                        // exit(), malloc(), rand()
#include <pthread.h>                                       // pthread_create(), pthread_join(), pthread_t
#include <time.h>                                          // clock(), clock_t
#include <semaphore.h>                                     // sem_t, sem_init(), sem_wait(), sem_post()

             
/* structure, enum, typdef */

typedef enum { False, True } bool;                         // Typdef boolean
             
struct Fin {char type[10], ext[10];};                      // Used to validate config/meta file

struct IO_D {                                              // Used to pass data to I/O thread
    int wmsec, procid; char code[5], desc[20];
}; 

enum INSTSTATUS {                                          // Indicates status of single meta instruction (for error checking)
    NULLMETA, EMPTYMETA, STARTMETA, ENDMETA, VALIDMETA
};
struct Instruction {                                       // Single meta-data instruction e.g. P{run}6
   enum INSTSTATUS status; int c, mstot; double msrem;
   char code[5], desc[20]; 
};

#define MAXPCBINUM 99
enum STATE {START, READY, RUNNING, WAITING, EXIT};         // pcb states
struct PCB {                                               // single PCB
    int id, priority, burstms, next, inum, ionum, icount;  // process id, priority, burst time, next inst, # total inst, # I/O inst, # current inst
    double remms;                                          // total remaining ms for Shortest Remaining Time scheduling
    enum STATE state;                                      // PCB State
    int imemblk, iswapblk, memblk[MAXPCBINUM];             // num of mem blocks allocated/swapped, address of mem blks allocated
    struct Instruction inst[MAXPCBINUM];                   // Instructions for process
}; 

struct SEM {                                               // Semaphore struct
    sem_t keyb, mon, mouse, hdd, print;                    // Semaphore keybaoard, monitor ...
    int curhdd, curprint;
};
              
struct CONFIG {                                            // Global configuration 
    int hdd, keyb, mem, mon, mouse, proc, print,           // cycle time (ms) of hdd, keyboard, memory...
    memkb, memblk, quant, hddq, printq;                    // memory/block size, quantum num, hdd/printer quantity
    char ver[20], fname_meta[50], fname_log[50], sched[9]; // version, meta/log filename, cpu scheduling code
} conf = {0,0,0,0,0,0,0,0,0,0,1,1,"","","",""};            // Default configuration              


/* Global Variables */ 

// ============= Miscellanous Semaphores for Interrupt and Load Meta =============
#define REPEATLOADMETA 5                                   // Number of times meta file is read and loaded (5 default)
sem_t sem_loadmeta, mut_interrupt;                         // sem_loadmeta = times meta file loaded, interrupt generated flag
sem_t mut_rq, mut_wq, mut_qtrans;                          // mutex ready queue, wait queue, PCB transferring from one queue to another
int Procid = 1, Metafid = 0, Proccount=0;                  // global count of process id, # meta file, total process count (maximum 1000)


// ============= I/O Device Semaphore =============
struct SEM sem;                                            // Blank semaphore struct
                                                                               
// ============= Log mode related global variables =============                                          
bool Log2mon = True, Log2file = False;                     // Flag - log to monitor, file
FILE *Flog;                                                // log file 

// ============= PCB READY Queue Global Variables =============
#define QMAX 99                                            // Maximum size of pcb READY queue
struct PCB readyq[QMAX];                                   // PCB READY queue as an array (min heap)
int inq = 0;                                               // Number of pcb (process) in READY queue

// ============= PCB WAITING Queue Global Variables =============
#define WQMAX 99                                           // Maximum size of PCB WAITING queue
struct PCB waitq[WQMAX];                                   // PCB WAITING queue as an array (min heap)
int inwq = 0;                                              // Number of pcb (process) in WAITING queue

// ============= Memory Heap Global Variables =============
#define MEMQMAX 99999                                      // Maximum number of memory blocks
int memq[MEMQMAX];                                         // FIFO queue of memory blocks
int memqfront = 0, memqrear = -1, inmemq = 0;              // front, rear and # of mem blocks in FIFO queue


/* Function Declarations */

// ========= Memory FIFO Queue & Swap Functions =========

bool memqisempty();                                        // True if memory full (memory block queue empty)
bool memqisfull();                                         // Memory address space = MEMQMAX blocks, error if more memory blocks
bool memqput(int memblk);                                  // put memory block at end of memory queue
int memqpeek();                                            // peek front of memory queue
int memqget();                                             // get memory block from front of memory queue
void memrecapture(struct PCB *pcb_ptr);                    // recapture memory allocated to process (used when process exits)
int memqswapnget();                                        // swap memory allocated to idle process into HDD and get it
void memswapback(struct PCB *pcb_ptr);                     // swap back memory of ready process from HDD
                                                           
// ========= PCB Ready Queue Functions =========           
bool qisempty();                                           // True if pcb ready queue empty
bool qisfull();                                            // True if pcb ready queue full
void qswap(int i, int j);                                  // Swap two processes (i,j) in PCB ready queue
void heapifyup();                                          // heapify ready queue starting from leaf
void heapifydown();                                        // heapify ready queue starting from root
void qput(struct PCB data);                                // put pcb in ready queue
struct PCB qget();                                         // get pcb from ready queue
struct PCB qpeek();                                        // peek into pcb ready queue
void qprint();                                             // print pcb ready queue as binary heap

// ========= PCB Ready Queue Functions =========           
bool wqisempty();                                          // True if pcb WAITING queue empty
bool wqisfull();                                           // True if pcb WAITING queue full
void wqswap(int i, int j);                                 // Swap two processes (i,j) in PCB WAITING queue
void wheapifyup();                                         // heapify WAITING queue starting from leaf
void wheapifydown();                                       // heapify WAITING queue starting from root
void wqput(struct PCB data);                               // put pcb in WAITING queue
struct PCB wqget();                                        // get pcb from WAITING queue
struct PCB wqpeek();                                       // peek into pcb WAITING queue
void wqprint();                                            // print pcb WAITING queue as binary heap
                                                           
// ========== Thead & Time Functions ===========            
double now();                                              // time (second) elapsed from start of program, current time 
double wait(double totms);                                 // countdown timer in millisecond, sleep for msec
double wait_proc(double totms);                            // wait for P{run}, with interrupt (maximum quantum ms if RR)
void *wait_io(void *arg);                                  // wait for I/O instruction
void *rr_timer(void *placeholder);                         // generate interrupt every quantum ms
                                                           
// ============= String Functions ==============           
int _atoi(const char *str);                                // convert string to integer (own impelementation of atoi)
bool isspace(int c);                                       // true if character c is whitespace
bool isempty(const char *str);                             // true if string str is empty (whitespace)
bool isinteger(const char *str);                           // true if string str is valid integer
int _strlen(const char *str);                              // number of characters in string (own impelementation of strlen)
bool streq(const char *s1, const char *s2);                // true if strings s1 and s2 are equal
void strtrim(char* str);                                   // removes leading/trailing/multiple whitespace
char *tolowerstr(char str[]);                              // make string str lowercase
char *strcpy(char *dest, const char *src);                 // copy value of string src to dst
char *strdup(const char *str);                             // copy string with proper memory allocation (strcpy with malloc)
char *strchr(const char *s, int c);                        // sets str pointer s to position of c in s
char *strsep(char **stringp, const char *delim);           // tokenize stringp at delimeters delim
                                                           
// ============== I/O Functions ================            
int _log(const char *format, ...);                         // log to monitor/file/both, replaces printf/fprintf
void logerror(const char *format, ...);                    // logs error and exit code
FILE *fopenval(const char* fname, const char *ftype, const char *fext);   // open and validate conf/meta file

// ============= Configuration File Related Functions =============
int verify(char *val, const char *part, const char *msg);  // validate integer values for cycle ms and memory kb in config file
void getconfig(FILE *fconf);                               // get config data from config file in a struct to use later

// ============= Meta-Data File Related Function s ==============
char *file2str(FILE *f);                                   // reads entire file into string
void codeVdesc(const char *token, const char *code, const char *desc);    // Validate meta-data code and descriptor
int verifycycle(const char *code, const char *desc, const char *cycle);   // Validate meta-data cycle value is positive integer
int getmstot(int c, const char *desc);                     // Calculate total msec for operation
bool isstartmeta(const char *token);                       // True if 'Start Program Meta-Data Code:'
bool isendmeta(const char *token);                         // True if 'End Program Meta-Data Code.'
struct Instruction getnextinst(char **buffer_ptr);         // Parse meta-instruction into Instruction 
void loadmeta();
void initsemnmem();                                        // Initialize memory, queuing and I/O semaphores
void *run_waitq(void *placeholder);                        // Simulates PCB in WAITING queue
void simulationstart();                                    // Helper function to simulate, starts executing each process
void simulate();                                           // Main Simulator Function, simulates a meta-data file


void main(int argc, char* argv[]) {
    //Config file 
    const char *fname_conf = argv[1];                      // Config filename
    FILE *fconf = fopenval(fname_conf, "config", ".conf"); // Open config file, extension must be .conf            
    getconfig(fconf);                                      // Store config in struct conf                         
    fclose(fconf);                                         // Close config file


    // Meta file
    simulate();
 
    if (Flog != NULL) fclose(Flog);                        // Close Log file if open                    

}


// ====================== Load Meta File =====================

bool isstartmeta(const char *token){                       // True if 'Start Program Meta-Data Code:' 
    char *copy = strdup(token);                            // copy of token
    strtrim(tolowerstr(copy));
    return streq(copy, "start program meta-data code");
}


bool isendmeta(const char *token){                         // True if 'End Program Meta-Data Code.'
    char *copy = strdup(token);                            // copy of token
    strtrim(tolowerstr(copy));
    return streq(copy, "end program meta-data code");
}


struct Instruction getnextinst(char **buffer_ptr){
    struct Instruction inst;                               // current meta-data instruction
    char *token, *copy;                                    // token = meta-data code e.g. A{begin}0
    char *code, *desc, *cycle;                             // meta-data code, descriptor, cycles
                                                           
    token = strsep(buffer_ptr, ":;.");                     // : = ; = . forgiving typo in meta file
                                                           
    if (isstartmeta(token)) inst.status = STARTMETA;       // Start Program Meta-Data Code
    else if (isendmeta(token)) inst.status = ENDMETA;      // End Program Meta-Data Code
    else if (token == NULL) inst.status = NULLMETA;        // NULL line means End of File
    else if (isempty(token)) inst.status = EMPTYMETA;      // Empty meta-data line
    else inst.status = VALIDMETA;                          
                                                           
    if (inst.status != VALIDMETA) return inst;             // Continue only if valid meta instruction     
                                                           
    copy = strdup(token);                                  // copy of token
    code = strsep(&copy, "{(");                            // meta code        
    desc = strsep(&copy, "})");                            // meta descriptor
    cycle = strsep(&copy, ")}");                           // meta cycles
                                                           
    strtrim(code); strtrim(desc); strtrim(cycle);          // remove whitespace
    desc = tolowerstr(desc);                               // descriptor is case insensitive
                                                           
    if (streq(desc, "start")) desc = strdup("begin");      // start == begin
    if (streq(desc, "end")) desc = strdup("finish");       // end == finish  
                                                           
    codeVdesc(token, code, desc);                          // Validate code and descriptor 
    int c = verifycycle(code, desc, cycle);                // Validate cycle value
    int mstot = getmstot(c, desc);                         // total msec for operation
                                                           
    strcpy(inst.code, code);                               // instruction code e.g. S, A, I, O
    strcpy(inst.desc, desc);                               // descriptor e.g. begin, printer, allocate
    inst.c = c;                                            // instruction cycles
    inst.mstot = mstot;                                    // instruction total cycle time in ms
    inst.msrem = (double) inst.mstot;
    
    //_log("\n ** %s{%s}%d - %d\n", inst.code, inst.desc, inst.c, inst.mstot); //debug
       
    return inst;
}


void initsemnmem(){
    sem_init(&sem_loadmeta, 0, 0);                         // number of times meta file has been loaded
    sem_init(&mut_interrupt, 0, 1);                        // flag=0 if interrupt generated by round robin timer thread
    sem_init(&mut_rq, 0, 1);                               // PCB READY queue read/write mutex to prevent race condition
    sem_init(&mut_qtrans, 0, 0);                           // if PCB is transferring from READY queue to WAITING queue or vice versa
    sem_init(&mut_wq, 0, 1);                               // PCB WAITING queue read/write mutex to prevent race condition
    
    
    for (int i=0; i < conf.memkb; i += conf.memblk){
        bool success = memqput(i);
        if (!success) logerror("\nWarning -- Number of memory blocks: %d, greater than system memory address space: %d!\n", conf.memkb/conf.memblk, MEMQMAX);
    }
    
    sem_init(&sem.keyb, 0, 1);                                                          
    sem_init(&sem.mon, 0, 1);
    sem_init(&sem.mouse, 0, 1);
    sem_init(&sem.hdd, 0, 1);
    sem_init(&sem.print, 0, 1);
    
    if (conf.hddq > 1){
        sem_destroy(&sem.hdd);
        sem_init(&sem.hdd, 0, conf.hddq);
    }
    
    if (conf.printq > 1){
        sem_destroy(&sem.print);
        sem_init(&sem.print, 0, conf.printq);
    }
    
    sem.curhdd=0; sem.curprint=0;                          // Keeps track of current PRIN or HDD to use in circular fashion
}


void loadmeta(){
    // Open, read, load, close meta file

    FILE *fmeta = fopenval(conf.fname_meta, "meta", ".mdf");// Open Meta file, extension must be .mdf
    
    char *buffer = file2str(fmeta);                        // Read meta file into buffer string
    struct Instruction inst;                               // Single meta instruction
    bool start=False, end=False;                           // Start/End Program Meta-Data Code
    static const struct PCB emptypcb={0,0,0,0,0,0,0,0,0,0,0};// Empty PCB
    struct PCB pcb = emptypcb;                             // pcb of current process
    int icount=0;                                          // instruction count, process id = arrival order
    bool oson=False, inproc=False;                         // Flags: OS turned on, reading a process
    
        do {
        inst = getnextinst(&buffer);
        
        //_log("\n-- %d, %s{%s}%d - %d\n", icount, inst.code, inst.desc, inst.c, inst.mstot); //debug
        
        if (inst.status==STARTMETA) {start=True; continue;}
        else if (inst.status==ENDMETA) {end=True; break;}  // ENDMETA = End of meta file
        else if (inst.status==NULLMETA) {break;}           // NULLMETA = (Abrupt) End of file, error
        if (!start || inst.status==EMPTYMETA) continue;    // Empty Line is allowed
        
        icount++;                                          // Total instruction count from beginning of meta file, used for error logging
        pcb.burstms = pcb.burstms + inst.mstot;            // burst time of process
        pcb.inst[pcb.inum] = inst;                         // store instruction in PCB instruction array
        pcb.inum++;                                        // instruction count of process
        if (pcb.inum == MAXPCBINUM) logerror("\nError in process # %d -- Exceeds maximum number of instructions: %d\n", Procid, MAXPCBINUM);
        
        if (*inst.code == 'S'){           
            pcb.id = 0;                                    // OS or System processes have process id 0
            
            if (streq(inst.desc, "begin")){
                if (oson)                                  // Error -- OS is already turned on
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot turn on OS because it is already on!\n", icount, inst.code, inst.desc, inst.c);
                
                oson = True;                               // OS on flag
                pcb.priority = 0;                          // S{begin} has 0 priority; stays at beginning of min heap
            } else if (streq(inst.desc, "finish")) { 
                if (!oson)                                 // Error -- OS is already shutdown
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot shutdown OS because it is already off!\n", icount, inst.code, inst.desc, inst.c);
                
                if (inproc)                                // Error -- OS cannot be shutdown while reading a process
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- OS Shutdown during process # %d!\n", icount, inst.code, inst.desc, inst.c, Procid);
                
                oson = False;                              // OS shutdown flag
                pcb.priority = INT_MAX;                    // S{finish} has INT_MAX priority; stays at end  of min heap
            }
            
            if (Metafid == 0 && streq(inst.desc, "begin")) {
                sem_wait(&mut_rq);
                qput(pcb);        
                sem_post(&mut_rq);
            }
            pcb = emptypcb;                                // Reinialize current pcb for next process
        } else if ( *inst.code == 'A') {
            if (streq(inst.desc, "begin")){
                if (!oson)                                 // Error -- process cannot be started while OS is shutdown
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot start process # %d because OS is shutdown!\n", icount, inst.code, inst.desc, inst.c, Procid);
                if (inproc)                                // Error -- new process cannot be started before finishing reading previous process
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot start reading new process before previous process terminates!\n", icount, inst.code, inst.desc, inst.c);
                
                inproc = True;
            } else if (streq(inst.desc, "finish")){
                if (!inproc)                               // Error -- cannot terminate because no process is started
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- There is no started process to terminate!\n", icount, inst.code, inst.desc, inst.c);
                inproc = False;                            // Flag: reading a process
                pcb.id = Procid;                           // Process id == order of arrival in meta file
                pcb.remms = pcb.burstms;
     
                // Heapsort is not stable by default. So Procid is added to priority
                // to maintain FIFO order for equal priority processes
                if (streq(conf.sched, "FIFO"))                    // FIFO, priority = process id = order of arrival
                    pcb.priority = pcb.id;                        
                else if (streq(conf.sched, "RR"))                 // RR, priority = process id = order of arrival
                    pcb.priority = pcb.id;                        
                else if (streq(conf.sched, "PS"))                 // PS, priority = 100 - number of I/O operations    
                    pcb.priority = 1000*(100-pcb.ionum) + pcb.id; // pcb.id (process id) ensures FIFO for equal ionum
                else if (streq(conf.sched, "SJF"))                // SJF, priority = inum = total number of operations    
                    pcb.priority = 1000*pcb.inum + pcb.id;        // 1000 ensures- process id does not offset inum
                else if (streq(conf.sched, "STF"))                // Shortest Time First, priority = burst time (ms)    
                    pcb.priority = 1000*pcb.burstms + pcb.id; 
                else if (streq(conf.sched, "STR"))                
                    pcb.priority = 1000*pcb.remms + pcb.id;       // STR, priority = remaining ms
                else logerror("\nError in config -- Unknown scheduler: %s", conf.sched);
                
                Procid++;                                         // Global process id count
                Proccount++;
                if (Procid == QMAX) logerror("\nError -- Meta file exceeds maximum number of process: %d\n", QMAX);
                
                sem_wait(&mut_rq);                                // acquire lock on PCB READY queue before writing
                qput(pcb);                                        // Put PCB in PCB ready queue (min heap)
                sem_post(&mut_rq);                                // relase lock on PCB READY queue after writing
                pcb = emptypcb;                                   // Reinialize PCB for next process
            }
        } else {
            if (!inproc)                                   // Error -- Instruction must be inside A{begin} - A{finish} block
                logerror("\nError in meta instruction # %d : %s{%s}%d -- Instruction is not inside a process block!\n", icount, inst.code, inst.desc, inst.c);
            
            if (*inst.code=='I' || *inst.code=='O') {
                pcb.ionum++;                               // I/O instruction count (used for Priority Schedule)
            }
        }       
        
    } while(1);                      
    
    if (!start) logerror("Error in meta file -- 'Start Program Meta-Data Code:' not present\n");
    if (!end) logerror("Error in meta file -- 'End Program Meta-Data Code.' not present\n");
    
    
    fclose(fmeta);                                         // Close Meta file
    
    sem_post(&sem_loadmeta);                               // increments sem_loadmeta to denote how many times meta file loaded
    Metafid++;                                             // ID of current meta file (0 to 4)
    
}


void *loadmeta_xtimes(void *placeholder){                  // Load meta file REPEATLOADMETA==5 times
    for (int i = 0; i < REPEATLOADMETA; i++){
        loadmeta();
        wait(100.0);
    }
}



// ====================== Simulator =====================

bool endsimulation(){                                      // True if all process simulated, end of simulation
    int loadmeta_status, qtrans_status;
    
    sem_getvalue(&sem_loadmeta, &loadmeta_status);         // How many times meta file has been loaded
    sem_getvalue(&mut_qtrans, &qtrans_status);             // How many PCB are being transferred from READY to WAITING queue or vice versa
    
    // End simulation if 1. meta file loaded 5 times 2. no PCB in transfer from one queue 
    // to another 3. no PCB in pcb READY queue 4. no PCB in pcb WAITING queue
    if (loadmeta_status==REPEATLOADMETA && qtrans_status==0 && qisempty() && wqisempty())
        return True;
    else
        return False;
}


void *run_waitq(void *placeholder){                        // Simulates PCB in WAITING queue
    struct PCB *pcbptr;
    
    pthread_t thread_id[WQMAX];
    int t = 0;
    
    while(!endsimulation()){
        
        if (wqisempty()) continue;                         // WAITING queue is empty but simulation is not over yet
        
        pcbptr = malloc( sizeof(struct PCB) );
        
        sem_post(&mut_qtrans);
        sem_wait(&mut_wq);
        *pcbptr = wqget();                                 // 
        sem_post(&mut_wq);
        
        pthread_create(&thread_id[t], NULL, wait_io, (void *) pcbptr);
        t++;
        if (t == WQMAX) t = 0;
    }
    for (t = 0; t < WQMAX; t++)
        pthread_join(thread_id[t], NULL);
}


void *run_readyq(void *placeholder){
    while(!endsimulation()){
        
        if (inq == 0) continue;
        
        sem_post(&mut_qtrans);
        sem_wait(&mut_rq);
        struct PCB pcb = qget();                                               
        sem_post(&mut_rq);
       
        int id = pcb.id;                                                       // process id (in order of arrival)       
        
        if (pcb.iswapblk > 0) memswapback(&pcb);                               // If memory of process was swapped before, reallocate
        
        while (pcb.icount < pcb.inum){
            struct Instruction it = pcb.inst[pcb.icount];
            
            char *code = strdup(it.code);
            char *desc = strdup(it.desc);
            int c = it.c;
            double wmsec = it.msrem;
            double mstot = (double) it.mstot;
            
            double elapsed = 0.0;
            
           //_log("\n-- %d -- %d, %s{%s}%d - %f %f %d\n", id, pcb.icount, code, desc, c, mstot, wmsec, wmsec==mstot); //debug
            
            if ( *code == 'S'){
                if ( streq(desc, "begin") ){
                    _log("%.6f - Simulator program starting\n", now());
                } else if ( streq(desc, "finish")){
                    _log("%.6f - Simulator Program ending\n", now());
                }
            } 
            else if ( *code == 'A' ){
                if ( streq(desc, "begin") ){
                    _log("%.6f - OS: preparing process %d\n", now(), id);
                    pcb.state = START;
                    _log("%.6f - OS: starting process %d\n", now(), id);
                    pcb.state = READY;
                } else if ( streq(desc, "finish")) {
                    memrecapture(&pcb);                                        // Recapture/free memory allocated to exiting process
                    _log("%.6f - OS: process %d completed\n", now(), id);
                    pcb.state = EXIT;
                }
            } 
            else if ( *code == 'P' ){
                pcb.state = RUNNING;
                
                if (wmsec == mstot)
                    _log("%.6f - Process %d: start processing action\n", now(), id);
                
                elapsed = wait_proc(wmsec);
                pcb.inst[pcb.icount].msrem = pcb.inst[pcb.icount].msrem - elapsed;
                pcb.remms = pcb.remms - elapsed;
                
                //_log("Elapsed: %f, Remaining: %f\n", elapsed, pcb.inst[pcb.icount].msrem); //debug
                
                if (pcb.inst[pcb.icount].msrem > 0){
                    pcb.state = READY;
                    _log("%.6f - Process %d: interrupt processing action\n", now(), id);
                    if (streq(conf.sched, "RR")) pcb.priority = pcb.priority + 1000;
                    sem_wait(&mut_rq);
                    qput(pcb);
                    sem_post(&mut_rq);
                    
                    break;
                } else {
                    _log("%.6f - Process %d: end processing action\n", now(), id);              
                }
            } 
            else if ( *code == 'I' || *code == 'O' ){
                pcb.state = WAITING;
                sem_wait(&mut_wq);
                wqput(pcb);
                sem_post(&mut_wq);
                break;
            } 
            else if (*code == 'M'){
                if (streq(desc, "block")){
                    _log("%.6f - Process %d: start memory blocking\n", now(), id);
                    wait(wmsec);
                    _log("%.6f - Process %d: end memory blocking\n", now(), id);
                } else if (streq(desc, "allocate")){
                    if (conf.memkb == 0)                                       // System Memory Size not given in config file
                        logerror("Error -- system memory size not given in config file\n");
                    if (conf.memblk == 0)                                      // System Memory Block Size not given in config file
                        logerror("Error -- memory block size not given in config file\n");
                    
                    _log("%.6f - Process %d: allocating memory\n", now(), id);
                    int loc = memqget();                                       // Get next available memory block from memory heap
                    if (loc == -1)                                             // System does not have enought memory for this process
                        logerror("\nError -- System ran out of Memory\n");     // Even after swapping out all the other processes
                    
                    pcb.memblk[pcb.imemblk++] = loc;                           // Keep track of memory blocks assigned to this process
                    
                    wait(wmsec);
                    //pcb.inst[icount].msrem = pcb.inst[icount].msrem - elapsed;
                    _log("%.6f - Process %d: memory allocated at 0x%08X\n", now(), id, loc);
                }
            }           
                        
            pcb.icount = pcb.icount+1;                      
        }
        sem_wait(&mut_qtrans);
        
    }
    _log("%.6f - Simulator Program ending\n", now());
}


void simulationstart(){
    
    if (streq(conf.sched, "RR")){
        pthread_t rr_timer_tid;
        pthread_create(&rr_timer_tid, NULL, rr_timer, NULL); 
    }
    
    pthread_t run_rq_tid, run_wq_tid;
    pthread_create(&run_rq_tid, NULL, run_readyq, NULL); 
    pthread_create(&run_wq_tid, NULL, run_waitq, NULL); 

    pthread_join(run_rq_tid, NULL);
    pthread_join(run_wq_tid, NULL);
}


void simulate(){
    initsemnmem();
    
    pthread_t loadmeta_tid;
    pthread_create(&loadmeta_tid, NULL, loadmeta_xtimes, NULL);
        
    simulationstart();                                     // Helper function to execute each PCB from ready queue  
        
}





// ========== Thead & Time Functions ==========

double now() {return (double) clock()/CLOCKS_PER_SEC;}


double wait(double totms){
    clock_t start = clock();
    int diff, totcycle = (int) (totms / 1000 * CLOCKS_PER_SEC);
    
    do {
        diff = (clock() - start);
    } while (diff < totcycle);
    
    double elapsed = ((double) diff) * 1000 / CLOCKS_PER_SEC;
    
    //_log("Tot ms: %f, tot cycle: %d, diff: %d, elapsed: %f\n", totms, totcycle, diff, elapsed ); //debug
    
    return totms;    
}


double wait_proc(double totms){
   // countdown timer in millisecond, sleep for msec
   
    int interrupt_status;
    double elapsed;
    sem_getvalue(&mut_interrupt, &interrupt_status);

    if (interrupt_status == 0 && totms > conf.quant) {
        elapsed = wait((double) conf.quant);
        sem_post(&mut_interrupt);
    } else {
        elapsed = wait(totms);
    }
    
    return elapsed;
}


void *wait_io(void *pcbptr){
    // entry point for I/O thread
    
    /*
    struct IO_D *io = (struct IO_D *) arg;
    double wmsec = (double) io->wmsec;
    int procid = io->procid;
    char *code = strdup(io->code), *desc = strdup(io->desc), *operation;
    */
    
    struct PCB pcb = *(struct PCB *) pcbptr;
    int procid = pcb.id;
    
    struct Instruction it = pcb.inst[pcb.icount];
    double wmsec = (double) it.mstot;
    char *code = strdup(it.code);
    char *desc = strdup(it.desc);
    char *operation, extramsg[50];
    
    if (streq(code, "I")) operation = strdup("input");
    else if (streq(code, "O")) operation = strdup("output");
    
    sem_t *lock;
    if (streq(desc, "keyboard")) lock = &sem.keyb;
    else if (streq(desc, "mouse")) lock = &sem.mouse;
    else if (streq(desc, "monitor")) lock = &sem.mon;
    else if (streq(desc, "hard drive")) lock = &sem.hdd;
    else if (streq(desc, "printer")) lock = &sem.print;
    
    
    if (streq(desc, "hard drive")){
        int curhdd = sem.curhdd++;
        if (sem.curhdd == conf.hddq) sem.curhdd = 0;
        sprintf(extramsg, " on HDD %d", curhdd);
    } else if (streq(desc, "printer")){
        int curprint = sem.curprint++;
        if (sem.curprint == conf.printq) sem.curprint = 0;
        sprintf(extramsg, " on PRIN %d", curprint);
    }
    
    
    sem_wait(lock);
    
    _log("%.6f - Process %d: start %s %s%s\n", now(), procid, desc, operation, extramsg);
    double elapsed = wait(wmsec);
    _log("%.6f - Process %d: end %s %s\n", now(), procid, desc, operation);

    sem_post(lock);
    
    pcb.icount = pcb.icount + 1;
    pcb.state = RUNNING;
    
    sem_wait(&mut_rq);
    qput(pcb);
    sem_post(&mut_rq);
    sem_wait(&mut_qtrans);
    
    free(pcbptr);

}


/*
void wait_io(struct IO_D *io_ptr){
    // create/join I/O thread, log and wait


    
    
    //pthread_join(thread_id, NULL); 
}
*/


void *rr_timer(void *placeholder){   
    while (1){
        sem_wait(&mut_interrupt);
    }
}






// ============= Meta-Data File Related Functions ==============

char *file2str(FILE *f){
    // Read meta file into buffer string
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *) malloc(length + 1);
    buffer[length] = '\0';
    fread(buffer, 1, length, f);
    if (!buffer) logerror("\nError -- reading meta-data file\n");

    return buffer;
}


void codeVdesc(const char *token, const char *code, const char *desc){
    // Validate meta-data code and descriptor combination
    char SA[][20] = {"begin", "finish"};        
    char P[][20]  = {"run"};
    char I[][20]  = {"hard drive", "keyboard", "mouse"};
    char O[][20]  = {"hard drive", "printer", "monitor"};
    char M[][20]  = {"block", "allocate", "cache"};
    
    char (*d)[20];
    if (*code == 'S' || *code == 'A') d = SA;
    else if (*code == 'P') d = P;
    else if (*code == 'I') d = I;
    else if (*code == 'O') d = O;
    else if (*code == 'M') d = M;
    else logerror("\nInvalid meta-data code '%s' in meta-data: '%s'\n", code, token);
    
    for (int i=0; i<3; i++){ 
        if (streq(desc, d[i])) return;
    }
    logerror("\nError -- Invalid descriptor '%s', for code %s, in meta-data '%s'\n", desc, code, token);
}


int verifycycle(const char *code, const char *desc, const char *cycle){
    if (isempty(cycle) && !streq(desc, "begin") && !streq(desc, "finish") ) 
        logerror("\nError in meta file -- missing cycle value for '%s' in: '%s{%s}%s'\n", desc, code, desc, cycle);
    
    if (!isinteger(cycle))
        logerror("\nError in meta file -- invalid (non-integer) cycles '%s' for '%s' in: '%s{%s}%s'\n", cycle, desc, code, desc, cycle);
    
    int c = _atoi(cycle);
    
    if ( (streq(desc, "begin") || streq(desc, "finish")) && c != 0 )
        logerror("\nError in meta file -- invalid (non-zero) cycles '%s' for '%s' in: '%s{%s}%s'\n", cycle, desc, code, desc, cycle);
    else if (c <= 0 && !streq(desc, "begin") && !streq(desc, "finish"))
        logerror("\nError in meta file -- invalid (non-positive) cycles '%s' for '%s' in: '%s{%s}%s'\n", cycle, desc, code, desc, cycle);

    return c;        
}


int getmstot(int c, const char *desc){
    // Calculate total msec for operation
    
    int mspc;            // millisesconds per cycle
    
    if (streq(desc, "hard drive"))    mspc = conf.hdd;
    else if (streq(desc, "keyboard")) mspc = conf.keyb;
    else if (streq(desc, "mouse"))    mspc = conf.mouse;
    else if (streq(desc, "monitor"))  mspc = conf.mon;
    else if (streq(desc, "run"))      mspc = conf.proc;
    else if (streq(desc, "allocate")) mspc = conf.mem;
    else if (streq(desc, "printer"))  mspc = conf.print;
    else if (streq(desc, "block"))    mspc = conf.mem;
    else if (streq(desc, "cache"))    mspc = conf.mem;
    else mspc = 0;
    
    if (mspc == 0 && !streq(desc, "begin") && !streq(desc, "finish"))
        logerror("\nError -- '%s' cycle time not given in config file\n", desc);
    
    return c*mspc;    // Total time for operation
}











// ============= Config File Related Functions =============

int verify(char *val, const char *part, const char *msg){
    // validate cycle msec or memory kbytes as positive number
    int m = 1;
    char *unit=0;
    tolowerstr(val);
       
    if ( !(streq(msg, "quantity")  )) { // || streq(msg, "quantum number")
        unit = strsep(&val, "})");      // unit = msec for cycle, kbytes/Mbytes/Gbytes for memory
        strsep(&val, ":");
        strtrim(unit); strtrim(val);    // remove whitespace
    }
        
    if (streq(msg, "size")){       
        if (streq(unit, "kbytes")) ;
        else if (streq(unit, "mbytes")) m = 1024;
        else if (streq(unit, "gbytes")) m = 1024*1024;
        else logerror("\nError -- Invalid unit for '%s' %s: %s\n", part, msg, unit);
    } 
    else if (streq(msg, "cycle time") && !streq(unit, "msec"))
        logerror("\nError in config file --  Invalid unit for '%s' %s: %s\n", part, msg, unit);
    
    if (isempty(val))                   // Missing cycle ms or memory kbytes
        logerror("\nError in config file -- Missing %s for '%s'\n", msg, part);
    
    if (!isinteger(val))                // non-integer cycle ms or memory kbytes 
        logerror("\nError in config file -- Invalid (non-integer) %s for '%s' : '%s'\n", msg, part, val);
    
    int v = _atoi(val);
    if (v <= 0)                         // Zero/Negative cycle ms or memory kbytes
        logerror("\nError in config file -- Invalid (non-positive) %s for '%s' : '%d'\n", msg, part, v);
    
    return v*m;                         // *m converts Mbytes/Gbytes to kbytes
}


void getconfig(FILE *fconf){
    // Returns config data in struct conf
     
    bool start=0, end=0, validlogmode=0, validlogfile=0;                             
    char line[999];                            
    int lineno = 0;
    
    while (fgets(line, sizeof(line), fconf)){
        lineno++;
  
        char *val = strdup(line);             
        char *key = strsep(&val, "{(:");                                      // key = before :  
        strtrim(tolowerstr(key)); strtrim(val); 

        if ( streq(key, "start simulator configuration file") ) {start = True; continue;}
        
        if ((isempty(key) && isempty(val)) || !start) continue;
        
        if (streq(key, "end simulator configuration file")) {end = True; break;}
        else if (isempty(val)) logerror("\nError config -- missing value for %s in line # %d", key, lineno);
        else if (streq(key, "hard drive cycle time"))    conf.hdd    = verify(val, "hard drive", "cycle time");
        else if (streq(key, "keyboard cycle time"))      conf.keyb   = verify(val, "keyboard", "cycle time");
        else if (streq(key, "memory cycle time"))        conf.mem    = verify(val, "memory", "cycle time");
        else if (streq(key, "monitor display time"))     conf.mon    = verify(val, "monitor", "display time");
        else if (streq(key, "mouse cycle time"))         conf.mouse  = verify(val, "mouse", "cycle time");
        else if (streq(key, "processor cycle time"))     conf.proc   = verify(val, "processor", "cycle time");
        else if (streq(key, "printer cycle time"))       conf.print  = verify(val, "printer", "cycle time");
        else if (streq(key, "hard drive quantity"))      conf.hddq   = verify(val, "hard drive", "quantity");
        else if (streq(key, "printer quantity"))         conf.printq = verify(val, "printer", "quantity");
        else if (streq(key, "system memory"))            conf.memkb  = verify(val, "system memory", "size");
        else if (streq(key, "memory block size"))        conf.memblk = verify(val, "memory block", "size");
        else if (streq(key, "processor quantum number")) conf.quant  = verify(val, "processor", "quantum number");
        else if (streq(key, "version/phase"))            strcpy(conf.ver, val);
        else if (streq(key, "file path"))                strcpy(conf.fname_meta, val);
        else if (streq(key, "cpu scheduling code"))      strcpy(conf.sched, val);
        else if (streq(key, "log")) {
            tolowerstr(val);
            if (streq(val, "log to file"))         { Log2mon = 0; Log2file = 1;}
            else if (streq(val, "log to monitor")) { Log2mon = 1; Log2file = 0;}
            else if (streq(val, "log to both"))    { Log2mon = 1; Log2file = 1;}
            else logerror("\nError -- Invalid logging mode: %s\n", val);
            validlogmode = 1;
        } else if (streq(key, "log file path")){
            strcpy(conf.fname_log, val);
            Flog = fopen(conf.fname_log, "w");
            validlogfile = 1;
        } else logerror("\nError -- Invalid config line: # %d\n", lineno);
    }

    if (isempty(conf.fname_meta))   // Meta file name not given
        logerror("\nError in config file -- no meta file specified!\n"); 
    
    if (!end)                       // Config file abruptly terminated
        logerror("\nError - config file abruptly terminated -- 'End Simulator Configuration File' not found!\n");
        
    if (Log2file && (!validlogfile)){
        printf("\nError in config -- Invalid or no logfile specified\n");
        exit(0);
    }

}










// ================ Memory FIFO Queue & Swapping ================

int memqpeek() {return memq[memqfront];}


bool memqisempty() { return inmemq==0;}


bool memqisfull() { return inmemq==MEMQMAX; }


bool memqput(int memblk){
    if (!memqisfull()){
        if (memqrear == MEMQMAX-1) memqrear = -1;
        memq[++memqrear] = memblk;
        inmemq++;
        return True;
    } else return False;
}


int memqget(){                          
    if (memqisempty()) return memqswapnget();
    
    int memblk = memq[memqfront++];
    if (memqfront == MEMQMAX) memqfront=0;
    inmemq--;
    return memblk;
}


void memrecapture(struct PCB *pcb_ptr){             // Recapture/free memory allocated to PCB
    for (int i = 0; i < pcb_ptr->imemblk; i++) {    
        memqput(pcb_ptr->memblk[i]);                
    }                                               
    pcb_ptr->imemblk = 0;                           
}                                                   
 
 
int memqswapnget(){                                 // If memory is full, swap memory of idle process into HDD
    for (int i = inq - 1; i > -1; i--){             
        if (readyq[i].imemblk > 0){                 
            readyq[i].iswapblk = readyq[i].imemblk; 
            memrecapture(&readyq[i]);               
            return memqget();                       
        }                                           
    }                                               
    return -1;                                      // Memory full, error
}                                                   


void memswapback(struct PCB *pcb_ptr){              // Reallocate memory to pcb because it was swapped to HDD earlier
    pcb_ptr->imemblk = pcb_ptr->iswapblk;
    for (int i = 0; i < pcb_ptr->imemblk; i++){
        pcb_ptr->memblk[i] = memqget();
    }
}



// ================= Min HEAP for PCB READY Queue ================

bool qisempty() { return inq == 0; }


bool qisfull() { return inq == QMAX; }


void qswap(int i, int j){
    struct PCB t = readyq[i];
    readyq[i] = readyq[j];
    readyq[j] = t;    
}


void heapifyup(){ 
    int child = inq-1, parent = child/2;
    while(parent >= 0 && readyq[parent].priority > readyq[child].priority) {
        qswap(parent, child);
        child = parent;
        parent = child/2;
    } 
}


void heapifydown(){ 
    int i = 0;
    while (i < inq){
        int min = i, left = 2*i+1, right = 2*i+2;
        if (left < inq && readyq[min].priority > readyq[left].priority) min = left;
        if (right < inq && readyq[min].priority > readyq[right].priority) min = right;
        if (min != i) qswap(min, i);
        else break;
        i = min;
    }
}


void qput(struct PCB pcb){
    readyq[inq++] = pcb;
    heapifyup();
}


struct PCB qpeek(){ return readyq[0]; }


struct PCB qget(){
    struct PCB pcb;
    if (qisempty()) return pcb;
    pcb = readyq[0];
    inq--;
    qswap(0, inq);
    heapifydown();
    return pcb;
}


void qprint(){
    for (int i=0; i < inq; i++) {
        _log("(%d, %d, %d) -- ", readyq[i].id, readyq[i].priority, readyq[i].inum);
    }
    _log("\n");
}



// ================= Min HEAP for PCB WAITING Queue ================

bool wqisempty() { return inwq == 0; }


bool wqisfull() { return inwq == WQMAX; }


void wqswap(int i, int j){
    struct PCB t = waitq[i];
    waitq[i] = waitq[j];
    waitq[j] = t;    
}


void wheapifyup(){ 
    int child = inwq-1, parent = child/2;
    while(parent >= 0 && waitq[parent].priority > waitq[child].priority) {
        wqswap(parent, child);
        child = parent;
        parent = child/2;
    } 
}


void wheapifydown(){ 
    int i = 0;
    while (i < inwq){
        int min = i, left = 2*i+1, right = 2*i+2;
        if (left < inwq && waitq[min].priority > waitq[left].priority) min = left;
        if (right < inwq && waitq[min].priority > waitq[right].priority) min = right;
        if (min != i) wqswap(min, i);
        else break;
        i = min;
    }
}


void wqput(struct PCB pcb){
    waitq[inwq++] = pcb;
    wheapifyup();
}


struct PCB wqpeek(){ return waitq[0]; }


struct PCB wqget(){
    struct PCB pcb;
    if (wqisempty()) return pcb;
    pcb = waitq[0];
    inwq--;
    qswap(0, inwq);
    wheapifydown();
    return pcb;
}


void wqprint(){
    for (int i=0; i < inwq; i++) {
        _log("(%d, %d) -- ", waitq[i].priority, waitq[i].inum);
    }
    _log("\n");
}




// =============== I/O Functions ===============

int _log(const char *format, ...){
    // log to monitor/file/both based on config
    // wrapper on fprintf, replaces printf & fprintf
    
    int r;
    va_list arg1, arg2;
    va_start(arg1, format);
	va_copy(arg2, arg1);
    if (Log2mon) { r = vfprintf(stdout, format, arg1); fflush(stdout);}           // stdout = monitor
    if (Log2file && Flog) { r = vfprintf(Flog, format, arg2); fflush(Flog);}    // global Flog
    va_end(arg1);
    return r;
}


void logerror(const char *format, ...){
    // log error message and exit
    
    va_list arg;
    va_start(arg, format);
    if (Log2mon) { vfprintf(stdout, format, arg); fflush(stdout);}
    if (Log2file && Flog) { vfprintf(Flog, format, arg); fflush(Flog);}
    va_end(arg);
    exit(0);
}


FILE *fopenval(const char *fname, const char *ftype, const char *fext){
    // Open & validate config or meta file
    // fname = filename, ftype = 'config' or 'meta'
    // fext = '.conf' for config file or '.meta' for metafile
    
    if (isempty(fname))                                                        // File name missing
        logerror("\nError -- Missing '%s' filename\n", ftype);
        
    char* ext = strchr(fname, '.');
    if ( !streq(ext, fext) )  
        logerror("\nError -- Invalid %s file extension: %s\n", ftype, ext);
    
    FILE* f = fopen(fname, "r");                                               // Open file 
    
    if (f == NULL)                                                             // File open failed
        logerror("\nError -- opening %s file, check filename: %s\n", ftype, fname);
    
    fseek (f, 0, SEEK_END);
    long fsize = ftell(f);                                                     // File size
    
    if (0 == fsize)                                                            // Empty file
        logerror("\nError -- %s file is empty\n", ftype);
    
    fseek(f, 0, SEEK_SET);                                                     // Reset file pointer
    return f;            
}



// ============= String Functions =============

int _atoi(const char *str) {
    // convert string to integer (own impelementation of atoi)
    
    char *s = strdup(str);
    strtrim(s);
    int sign = 1, num = 0, i =0;

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s=='+') {
        s++;
    }
    
    while (*s){
        if (*s < '0' || *s > '9') {logerror("\nError in _atoi -- Invalid Integer: %s %s\n", s, str);}
        num = num *10 + *s - 48;
        s++;
    }

    return (num * sign);
}


bool isspace(int c) {return ((c>=0x09 && c<=0x0D) || (c==0x20));}


bool isempty(const char *str) {
    if (str==NULL) return True;
    char *copy = strdup(str);
    strtrim(copy);
    if (copy[0]==' ' && copy[1]==0) _log("\nNot gonna happen\n"); 
    return (copy==NULL || copy[0]==0 || (copy[0]==' ' && copy[1]==0));
}


bool isinteger(const char *str) {
    // true if string str is valid integer
    
    if (*str == '+' || *str == '-') str++;
    while(*str){
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}


int _strlen(const char *str){
   // number of characters in string (own impelementation of strlen)
   if (!str) return 0;
   const char *ptr = str;
   while (*str) ++str;
   return str - ptr;
}


bool streq(const char *s1, const char *s2) {
    // true if strings s1 and s2 are equal
    
    if (s1 == NULL || s2 == NULL) { return 0; }
    while (*s1 != '\0' && *s2 != '\0'  && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*s1 - *s2) == 0;
}


void strtrim(char* str) {
    // remove leading/trailing/multiple whitespace from str
    
    if (str == NULL) return;
    int i=0, x;
    while (str[i] != '\0'){
        if (isspace(str[i])) str[i] = ' ';
        i++;
    }
    for(i=x=0; str[i]; ++i)
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
            str[x++] = str[i];
    str[x] = '\0';
    if (isspace(str[x-1])) str[x-1] = '\0';
}


char *tolowerstr(char str[]){
    // make str lowercase
    
    if (str == NULL) return str;
    for(int i = 0; str[i]; i++) 
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] = str[i] + 'a' - 'A';
    return str;
}


char *strcpy(char *dst, const char *src) {
    // copy value of string src to dst
    
   char *save = dst;
   while(*dst++ = *src++);
   return save;
}


char *strdup(const char *str) {
    // copy string with proper memory allocation (strcpy with malloc)
    
    if (str == NULL) return NULL;
    char *copy = malloc(_strlen(str) + 1);
    if (copy != NULL)
        return strcpy(copy, str);
}


char *strchr(const char *s, int c) {
    // sets str pointer s to position of c in s
    
    while (s[0] != c && s[0] != '\0') s++;
    if (s[0] == '\0')  return NULL;
    return (char *)s;
}


char *strsep(char **stringp, const char *delim) {
    // tokenize stringp at delimeters delim
    
    if (*stringp == NULL)  return NULL;
    char *start, *end;
    start = end = *stringp;
    while (end[0] != '\0' && strchr(delim, end[0]) == NULL) 
        end++;
    if (end[0] == '\0') 
        *stringp = NULL; 
    else {
        end[0] = '\0';
        *stringp = &end[1];
    }
    return start;
}

