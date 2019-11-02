/**
* sim4.c
*
* Project 4
*
* CS 646 Fall 2019
* 
* To compile, run makefile by entering "make"
*
* To run ./sim4 sim4.conf
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
#define MEMQMAX 999                                        // Maximum number of memory blocks
                 
typedef enum { False, True } bool;                         // Typdef boolean
             
struct Fin {char type[10], ext[10];};                      // Used to validate config/meta file

struct IO_D {                                              // Used to pass data to I/O thread
    int msec, procid; char *code, *desc; 
}; 

enum INSTSTATUS {                                          // Indicates status of single meta instruction (for error checking)
    NULLMETA, EMPTYMETA, STARTMETA, ENDMETA, VALIDMETA
};
struct Instruction {                                       // Single meta-data instruction e.g. P{run}6
   enum INSTSTATUS status; int c, mstot; char code[5], desc[20]; 
};

#define INSTMAX 99
enum STATE {START, READY, RUNNING, WAITING, EXIT};         // pcb states
struct PCB {                                               // single PCB
    int id, priority, burstms, next, inum, ionum;          // process id, priority, burst time, next instruction, total instructions, number of I/O operations
    enum STATE state; int imemblk, iswapblk, memblk[50];   // pcb state, num of mem blocks allocated/swapped, address of mem blks allocated
    struct Instruction inst[INSTMAX];                      // Instructions for process
}; 

struct SEM {                                               // Semaphore struct
    sem_t keyb, mon, mouse, hdd, print;                    // Semaphore keybaoard, monitor ...
};
              
struct CONFIG {                                            // Global configuration 
    int hdd, keyb, mem, mon, mouse, proc, print,           // cycle time (ms) of hdd, keyboard, memory...
    memkb, memblk, quant, hddq, printq;                    // memory/block size, quantum num, hdd/printer quantity
    char ver[20], fname_meta[50], fname_log[50], sched[9]; // version, meta/log filename, cpu scheduling code
} conf = {0,0,0,0,0,0,0,0,0,0,1,1,"","","",""};            // Default configuration              


/* Global Variables */ 

// ============= I/O Device Semaphore =============
struct SEM sem;                                            // Blank semaphore struct
                                                                               
// ============= Log mode related global variables =============                                          
bool Log2mon = True, Log2file = False;                     // Flag - log to monitor, file
FILE *Flog;                                                // log file 

// ============= PCB Ready Queue Global Variables =============
#define QMAX 99                                            // Maximum size of pcb ready queue
struct PCB readyq[QMAX];                                   // PCB ready queue as an array (min heap)
int inq = 0;                                               // Number of pcb (process) in ready queue

// ============= Memory Heap Global Variables =============
int memq[MEMQMAX];
int inmemq = 0;


/* Function Declarations */

// ========= Memory Heap & Swap Functions =========

bool memqisempty();                                        // True if memory full (memory heap empty)
bool memqisfull();                                         // True if memory size greater than system memory address space
void memqswap(int i, int j);                               // Exchange two memory blocks in memory heap
void memheapifyup();                                       // heapify memory heap starting from leaf
void memheapifydown();                                     // heapify memory heap starting from root
void memqput(int memblk);                                  // put memory block into heap
int memqpeek();                                            // peek element in memory block
int memqget();                                             // get memory block from heap
void memqprint();                                          // print memory heap
void memrecapture(struct PCB *pcb_ptr);                    // recapture memory allocated to process 
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
                                                           
// ========== Thead & Time Functions ===========            
double now();                                              // time (second) elapsed from start of program, current time 
void wait(int msec);                                       // countdown timer in millisecond, sleep for msec
void *wait_t(void *arg);                                   // entry point for I/O thread
pthread_t io_thread(struct IO_D *io_ptr);                  // create/join I/O thread, log and wait
                                                           
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
struct Instruction getnextinst(char **buffer_ptr);         // Parse meta-instruction into Instruction struct
void initsemnmem();                                        // Initialize memory heap and I/O semaphores
void simulationstart();                                    // Helper function to simulate, starts executing each process
void simulate(FILE *fmeta);                                // Main Simulator Function, simulates a meta-data file




int main(int argc, char* argv[]) {
    //Config file 
    const char *fname_conf = argv[1];                      // Config filename
    FILE *fconf = fopenval(fname_conf, "config", ".conf"); // Open config file, extension must be .conf            
    getconfig(fconf);                                      // Store config in struct conf                         
    fclose(fconf);                                         // Close config file
                         
    // Meta file
    FILE *fmeta;                                           // Meta file
    fmeta = fopenval(conf.fname_meta, "meta", ".mdf");     // Open Meta file, extension must be .mdf                  
    simulate(fmeta);                                       // Simulate meta-file instructions                      
    fclose(fmeta);                                         // Close Meta file
    
    if (Flog != NULL) fclose(Flog);                        // Close Log file if open                    

    return 1;
}



// ====================== Simulator =====================

bool isstartmeta(const char *token){
    char *copy = strdup(token);                       // copy of token
    strtrim(tolowerstr(copy));
    return streq(copy, "start program meta-data code");
}


bool isendmeta(const char *token){
    char *copy = strdup(token);                       // copy of token
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
    
    return inst;
}


void initsemnmem(){
    for (int i=0; i < conf.memkb; i += conf.memblk){
        memqput(i);
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
}


void simulationstart(){
    initsemnmem();

    while (!qisempty()){
        struct PCB pcb = qget();                                               // Get PCB (of highest priority) from ready queue
        int id = pcb.id;                                                       // process id (in order of arrival)
        int icount = 0;                                                        // total instructions/tasks in process
        
        if (pcb.iswapblk > 0) memswapback(&pcb);                               // If memory of process was swapped before, reallocate
        
        for (icount = 0; icount < pcb.inum; icount++){
            struct Instruction it = pcb.inst[icount];
            
            char *code = strdup(it.code);
            char *desc = strdup(it.desc);
            int c = it.c;
            int mstot = it.mstot;
            
           // _log("\n-- %d, %s{%s}%d - %d\n", icount, code, desc, c, mstot); //debug
            
            if ( *code == 'S'){
                if ( streq(desc, "begin") ){
                    _log("%.6f - Simulator program starting\n", now());
                } else if ( streq(desc, "finish")){
                    _log("%.6f - Simulator Program ending\n", now());
                }
            } else if ( *code == 'A' ){
                if ( streq(desc, "begin") ){
                    _log("%.6f - OS: preparing process %d\n", now(), id);
                    pcb.state = START;
                    _log("%.6f - OS: starting process %d\n", now(), id);
                    pcb.state = READY;
                } else if ( streq(desc, "finish")) {
                    memrecapture(&pcb);                                        // Recapture/free memory allocated to exiting process
                    _log("%.6f - End process %d\n", now(), id);
                    pcb.state = EXIT;
                }
            } else if ( *code == 'P' ){
                pcb.state = RUNNING;
                _log("%.6f - Process %d: start processing action\n", now(), id);
                wait(mstot);
                _log("%.6f - Process %d: end processing action\n", now(), id);
            } else if ( *code == 'I' || *code == 'O' ){
                struct IO_D iod = {mstot, id, code, desc};
                pcb.state = WAITING;
                io_thread(&iod);                                               // I/O operations are threaded in this function
                pcb.state = RUNNING;
            } else if (*code == 'M'){
                if (streq(desc, "block")){
                    _log("%.6f - Process %d: start memory blocking\n", now(), id);
                    wait(mstot);
                    _log("%.6f - Process %d: end memory blocking\n", now(), id);
                } else if (streq(desc, "allocate")){
                    if (conf.memkb == 0)                                       // System Memory Size not given in config file
                        logerror("Error -- system memory size not given in config file");
                    if (conf.memblk == 0)                                      // System Memory Block Size not given in config file
                        logerror("Error -- memory block size not given in config file");
                    
                    _log("%.6f - Process %d: allocating memory\n", now(), id);
                    int loc = memqget();                                       // Get next available memory block from memory heap
                    if (loc == -1)                                             // System does not have enought memory for this process
                        logerror("\nError -- System ran out of Memory");       // Even after swapping out all the other processes
                    
                    pcb.memblk[pcb.imemblk++] = loc;                           // Keep track of memory blocks assigned to this process
                    
                    wait(mstot);
                    _log("%.6f - Process %d: memory allocated at 0x%08X\n", now(), id, loc);
                }
            }
            
        }     
    }
}


void simulate(FILE *fmeta){
    // Read Meta-file into PCB ready queue
    char *buffer = file2str(fmeta);                        // Read meta file into buffer string
    char *buffercopy = buffer;
    struct Instruction inst;                               // Single meta instruction
    bool start=False, end=False;                           // Start/End Program Meta-Data Code
    
    static const struct PCB emptypcb={0,0,0,0,0,0,0,0,0};  // Empty PCB
    struct PCB pcb = emptypcb;                             // pcb of current process
    int icount=0, procid=1;                                // instruction count, process id = arrival order
    bool oson=False, inproc=False;                         // Flags: OS turned on, reading a process
    
    do {
        inst = getnextinst(&buffer);
        
        if (inst.status==STARTMETA) {start=True; continue;}
        else if (inst.status==ENDMETA) {end=True; break;}
        if (!start || inst.status==EMPTYMETA) continue;
        
        icount++;                                          // Total instruction count from beginning of meta file, used for error logging
        pcb.burstms = pcb.burstms + inst.mstot;            // burst time of process
        pcb.inst[pcb.inum] = inst;                         // store instruction in PCB instruction array
        pcb.inum++;                                        // instruction count of process
        
        if (*inst.code == 'S'){           
            pcb.id = 0;                                    // OS or System processes have process id 0
            
            if (streq(inst.desc, "begin")){
                if (oson)                                  // Error -- OS is already turned on
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot turn on OS because it is already on!", icount, inst.code, inst.desc, inst.c);
                
                oson = True;                               // OS on flag
                pcb.priority = 0;                          // S{begin} has 0 priority; stays at beginning of min heap
            } else if (streq(inst.desc, "finish")) { 
                if (!oson)                                 // Error -- OS is already shutdown
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot shutdown OS because it is already off!", icount, inst.code, inst.desc, inst.c);
                
                if (inproc)                                // Error -- OS cannot be shutdown while reading a process
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- OS Shutdown during process # %d!", icount, inst.code, inst.desc, inst.c, procid);
                
                oson = False;                              // OS shutdown flag
                pcb.priority = INT_MAX;                    // S{finish} has INT_MAX priority; stays at end  of min heap
            }
            
            qput(pcb);                                     // Put PCB in ready queue (min heap)
            pcb = emptypcb;                                // Reinialize current pcb for next process
        } else if ( *inst.code == 'A') {
            if (streq(inst.desc, "begin")){
                if (!oson)                                 // Error -- process cannot be started while OS is shutdown
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot start process # %d because OS is shutdown!", icount, inst.code, inst.desc, inst.c, procid);
                
                if (inproc)                                // Error -- new process cannot be started before finishing reading previous process
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- Cannot start reading new process before previous process terminates!", icount, inst.code, inst.desc, inst.c);
                
                inproc = True;
            } else if (streq(inst.desc, "finish")){
                if (!inproc)                               // Error -- cannot terminate because no process is started
                    logerror("\nError in meta instruction # %d : %s{%s}%d -- There is no started process to terminate!", icount, inst.code, inst.desc, inst.c);
                
                inproc = False;                            // Flag: reading a process
                pcb.id = procid;                           // Process id == order of arrival in meta file
                
                // Heapsort is not stable by default. So procid is added to priority
                // to maintain FIFO order for equal priority processes
                if (streq(conf.sched, "FIFO"))             
                    pcb.priority = pcb.id;                        // FIFO, priority = process id = order of arrival
                else if (streq(conf.sched, "PS"))                 // PS, priority = 100 - number of I/O operations    
                    pcb.priority = 1000*(100-pcb.ionum) + pcb.id; // pcb.id (process id) ensures FIFO for equal ionum
                else if (streq(conf.sched, "SJF"))                // SJF, priority = inum = total number of operations    
                    pcb.priority = 1000*pcb.inum + pcb.id;        // 1000 ensures- process id does not offset inum
                else if (streq(conf.sched, "STF"))                // Shortest Time First, priority = burst time (ms)    
                    pcb.priority = 1000*pcb.burstms + pcb.id; 
                else logerror("\nError in config -- Unknown scheduler: %s", conf.sched);
                
                procid++;                                  // process id count
                qput(pcb);                                 // Put PCB in PCB ready queue (min heap)
                pcb = emptypcb;                            // Reinialize PCB for next process
            }
        } else {
            if (!inproc)                                   // Error -- Instruction must be inside A{begin} - A{finish} block
                logerror("\nError in meta instruction # %d : %s{%s}%d -- Instruction is not inside a process block!", icount, inst.code, inst.desc, inst.c);
            
            if (*inst.code=='I' || *inst.code=='O') {
                pcb.ionum++;                               // I/O instruction count (used for Priority Schedule)
            }
        }       
        
    } while(inst.status != NULLMETA);                      // NULLMETA = End of file
    
    if (!start) logerror("Error in meta file -- 'Start Program Meta-Data Code:' not present");
    if (!end) logerror("Error in meta file -- 'End Program Meta-Data Code.' not present");
    
    simulationstart();                                     // Helper function to execute each PCB from ready queue
            
}



// ================ Memory Heap & Swapping ================

/*
bool memqisempty() { return inmemq == 0; }


bool memqisfull() { return inmemq == MEMQMAX; }


void memqswap(int i, int j){
    int t = memq[i];
    memq[i] = memq[j];
    memq[j] = t;    
}


void memheapifyup(){ 
    int child = inmemq-1, parent = child/2;
    while(parent >= 0 && memq[parent] > memq[child]) {
        memqswap(parent, child);
        child = parent;
        parent = child/2;
    } 
}


void memheapifydown(){ 
    int i = 0;
    while (i < inmemq){
        int min = i, left = 2*i+1, right = 2*i+2;
        if (left < inmemq && memq[min] > memq[left]) min = left;
        if (right < inmemq && memq[min] > memq[right]) min = right;
        if (min != i) memqswap(min, i);
        else break;
        i = min;
    }
}


void memqput(int memblk){
    memq[inmemq++] = memblk;
    memheapifyup();
}


int memqpeek(){ return memq[0]; }


int memqget(){
    if (memqisempty()) return memqswapnget();
    
    int memblk = memq[0];
    inmemq--;
    memqswap(0, inmemq);
    memheapifydown();
    return memblk;
}


void memqprint(){
    for (int i=0; i < inmemq; i++) {
        _log("%d -- ", memq[i]);
    }
    _log("\n");
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

*/

// ================= Min HEAP for PCB Ready Queue ================

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
        _log("(%d, %d) -- ", readyq[i].priority, readyq[i].inum);
    }
    _log("\n");
}



// ========== Thead & Time Functions ==========

double now() {return (double)clock()/CLOCKS_PER_SEC;}


void wait(int msec){
   // countdown timer in millisecond, sleep for msec
    
    clock_t before = clock(), diff;
    double ms;
    do {
      diff = (double) (clock() - before);
      ms = diff * 1000 / CLOCKS_PER_SEC;
    } while ( ms < msec );
}


void *wait_t(void *arg){
    // entry point for I/O thread
    
    struct IO_D io = *(struct IO_D*) arg;
    int msec = io.msec, procid = io.procid;
    char *code = io.code, *desc = io.desc, *operation;
    
    if (streq(io.code, "I")) operation = strdup("input");
    else if (streq(io.code, "O")) operation = strdup("output");
    
    sem_t *lock;
    if (streq(desc, "keyboard")) lock = &sem.keyb;
    else if (streq(io.desc, "mouse")) lock = &sem.mouse;
    else if (streq(io.desc, "monitor")) lock = &sem.mon;
    else if (streq(io.desc, "hard drive")) lock = &sem.hdd;
    else if (streq(io.desc, "printer")) lock = &sem.print;
    
    sem_wait(lock);
    
    _log("%.6f - Process %d: start %s %s\n", now(), io.procid, io.desc, operation);
    wait(io.msec);
    _log("%.6f - Process %d: end %s %s\n", now(), io.procid, io.desc, operation);

    sem_post(lock);
}


pthread_t io_thread(struct IO_D *io_ptr){
    // create/join I/O thread, log and wait
    
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, wait_t, io_ptr); 
    pthread_join(thread_id, NULL); 
    return thread_id;
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
    
    
    
    if ( !(streq(msg, "quantity")  || streq(msg, "quantum number"))) {
        unit = strsep(&val, "})");      // unit = msec for cycle, kbytes/Mbytes/Gbytes for memory
        strsep(&val, ":");
        strtrim(unit); strtrim(val);    // remove whitespace
    }
        
    if (streq(msg, "size")){       
        if (streq(unit, "kbytes")) ;
        else if (streq(unit, "mbytes")) m = 1024;
        else if (streq(unit, "gbytes")) m = 1024*1024;
        else logerror("\nError -- Invalid unit for '%s' %s: %s", part, msg, unit);
    } 
    else if (streq(msg, "cycle time") && !streq(unit, "msec"))
        logerror("\nError in config file --  Invalid unit for '%s' %s: %s", part, msg, unit);
    
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

    if (isempty(conf.fname_meta)) // Meta file name not given
        logerror("\nError in config file -- no meta file specified!\n"); 
    
    if (!end)                       // Config file abruptly terminated
        logerror("\nError - config file abruptly terminated -- 'End Simulator Configuration File' not found!\n");
        
    if (Log2file && (!validlogfile)){
        printf("\nError in config -- Invalid or no logfile specified\n");
        exit(0);
    }
        
}



// =============== I/O Functions ===============

int _log(const char *format, ...){
    // log to monitor/file/both based on config
    // wrapper on fprintf, replaces printf & fprintf
    
    int r;
    va_list arg1, arg2;
    va_start(arg1, format);
	va_copy(arg2, arg1);
    if (Log2mon) { r = vfprintf(stdout, format, arg1); }     // stdout = monitor
    if (Log2file && Flog) { r = vfprintf(Flog, format, arg2); }      // global Flog
    va_end(arg1);
    return r;
}


void logerror(const char *format, ...){
    // log error message and exit
    
    va_list arg;
    va_start(arg, format);
    if (Log2mon) { vfprintf(stdout, format, arg); }
    if (Log2file && Flog) { vfprintf(Flog, format, arg); }
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

