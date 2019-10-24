/**
* sim3.c
*
* Project 3
*
* CS 646 Fall 2019
* 
* To compile, run makefile by entering "make"
*
* To run ./sim3 sim3.conf
*
* Farhan Sadique
*/


#include <stdarg.h>                               // va_start(), va_end()
#include <stdio.h>                                // vprintf(), FILE(), fopen() ...
#include <stdlib.h>                               // exit(), malloc(), rand()
#include <pthread.h>                              // pthread_create(), pthread_join(), pthread_t
#include <time.h>                                 // clock(), clock_t
#include <string.h>

                                         					  
/* Structure, Enums, definitions */     
typedef enum { FALSE, TRUE } bool;                // Typdef boolean              
struct Fin {char type[10], ext[10];};             // Used to validate config/meta file
struct Io_d {                                     // Used to pass data to I/O thread
    int msec, procid; char *code, *desc; 
};                                                
struct Config {                                   // Configuration 
    int monitor, processor, mouse, hdd, keyboard, memory, printer, memkb, memblock, printer_q, hdd_q;
    char version[10], meta_fname[50], logmode[20], log_fname[50]; 
};                                                
struct Instruction {                              // Single meta-data instruction e.g. P{run}6
    int c, mstot; char code[2], desc[11]; 
};
enum pcb_s {START, READY, RUNNING, WAITING, EXIT};// pcb states
struct PCB {                                      // single PCB
    int id, burstms, next, inum; 
    enum pcb_s state;
    struct Instruction inst[200];
};    


/* Global Variables */

// Log mode related global variables
bool log2mon = 1, log2file = 0;                   // Flag - log to monitor, file
char log_fname[50];                               // log file name
FILE *logfile;                                    // log file 

// Semaphore and mutex global variables
unsigned int printsem = 1;                        // printer semaphore
unsigned int hddsem = 1;                          // HDD semaphore
unsigned int curprint = 0;                        // current printer in use
unsigned int curhdd = 0;                          // current HDD in use
                                                  
unsigned int monmut = 1;                          // monitor mutex
unsigned int mousemut = 1;                        // mouse mutex
unsigned int keybmut = 1;                         // keyboard mutex

// Configuration global variable
struct Config conf = {0,0,0,0,0,0,0,0,0,1,1,"None","None","None","None"};

// PCB ready queue global variables
#define QMAX 50
struct PCB readyq[QMAX];
int front = 0;
int rear = -1;
int inq = 0;                                                  
                                                  
// ============ Function Declarations ==========  
                   
// ========= PCB Ready Queue Functions =========
struct PCB qpeek();                               // peek into pcb ready queue
bool qisempty();                                  // is pcb ready queue empty
bool qisfull();                                   // is pcb ready queue full
int qsize();                                      // number of process in ready queue
void qput(struct PCB data);                       // put pcb in ready queue
struct PCB qget();                                // get pcb from ready queue
                                                  
// ========== Thead & Time Functions ===========   
double now();                                     // time (second) elapsed from start of program, current time 
void wait(int msec);                              // countdown timer in millisecond, sleep for msec
void *wait_t(void *arg);                          // entry point for I/O thread
void io_thread(struct Io_d *io_ptr);              // create/join I/O thread, log and wait

// ============= String Functions ==============
int _atoi(const char *str);                       // convert string to integer (own impelementation of atoi)
bool isspace(int c);                              // true if character c is whitespace
bool isempty(const char *str);                    // true if string str is empty (whitespace)
bool isinteger(const char *str);                  // true if string str is valid integer
int _strlen(const char *str);                     // number of characters in string (own impelementation of strlen)
bool streq(const char *s1, const char *s2);       // true if strings s1 and s2 are equal
void strtrim(char* str);                          // removes leading/trailing/multiple whitespace
char *tolowerstr(char str[]);                     // make string str lowercase
char *strcpy(char *dest, const char *src);        // copy value of string src to dst
char *strdup(const char *str);                    // copy string with proper memory allocation (strcpy with malloc)
char *strchr(const char *s, int c);               // sets str pointer s to position of c in s
char *strsep(char **stringp, const char *delim);  // tokenize stringp at delimeters delim
                                                  
// ============== I/O Functions ================   
int _log(const char *format, ...);                // log to monitor/file/both, replaces printf/fprintf
void logerror(const char *format, ...);           // logs error and exit code
FILE *fopenval(const char* fname, struct Fin fin);// open and validate conf/meta file
void setlogmode(FILE *fconf);                     // sets universal log mode to monitor/file/both from config file

// ============= Configuration File Related Functions =============
void startconf(FILE *fconf);                      // detect 'Start Simulator Configuration File', raise error otherwise
int verify(char *val, const char *part, const char *msg); // validate integer values for cycle ms and memory kb in config file
struct Config getconfig(FILE *fconf);             // qget config data from config file in a struct to use later
                                                  
// ============= Meta-Data File Related Function s ==============
char *file2str(FILE *f);                          // reads entire file into string
char *startmeta(char *buffer);                    // Detect 'Start Program Meta-Data Code:', raise error otherwise
void codeVdesc(const char *token, const char *code, const char *desc);    // Validate meta-data code and descriptor
int verifycycle(const char *code, const char *desc, const char *cycle);   // Validate meta-data cycle value is positive integer
int getmstot(int c, const char *desc, struct Config conf);// Calculate total msec for operation
void simulate(FILE *fmeta, struct Config conf);   // Main Simulator Function, simulates a meta-data file



// main function 
int main(int argc, char* argv[]) {    
    struct Fin fin_conf = {"config", ".conf"},    // config file must have .conf ext
    fin_meta = {"meta", ".mdf"};                  // meta file must have .mdf extension
    
    // Config file 
    const char* config_fname = argv[1];           // Config filename
    FILE* fconf;                                  // Config file
    fconf = fopenval(config_fname,fin_conf);      // Open Config file
    
    setlogmode(fconf);                            // set log mode to monitor/file/both
    
    struct Config conf = getconfig(fconf);        // log config and store in struct conf
    
    fclose(fconf);                                // Close config file

    // Meta file 
    FILE * fmeta;                                 // Meta file
    fmeta = fopenval(conf.meta_fname, fin_meta);  // Open Meta file
    
    simulate(fmeta, conf);                        // Simulate meta-file instructions
    
    fclose(fmeta);                                // Close Meta file
    
    if (logfile != NULL) fclose(logfile);         // Close Log file if open                    
    
    return 1;
}


// Main Simulator Function
void simulate(FILE *fmeta, struct Config conf){
    // Read Meta-file into PCB ready queue
    char *buffer = file2str(fmeta);    // Read meta file into buffer string    
    buffer = startmeta(buffer);        // Detect Start Program Meta-Data Code: 
    char *token, *copy;                // token = meta-data code e.g. A{begin}0
    char *code, *desc, *cycle;         // meta-data code, descriptor, cycles
              
    static const struct PCB emptypcb={0,0,0,0,0};
    struct PCB pcb = emptypcb;         // pcb of current process
    int burstms, icount, procid = 1;   // burst time, instruction count, id of process
    bool validend = FALSE;             // True if End Program Meta-Data Code. detected
    bool inproc=0;
    
    while ((token = strsep(&buffer, ":;.")) != NULL) {
        strtrim(token);                                   // remove whitespace
        copy = strdup(token);                             // copy of token
        tolowerstr(copy);                                 // End program meta-data is case-insensitive

        if (streq(copy, "end program meta-data code")){   // end of meta file
            validend = TRUE;
            break;
        }
        
        copy = strdup(token);                             // copy of token
        code = strsep(&copy, "{(");                       // meta code        
        desc = strsep(&copy, "})");                       // meta descriptor
        cycle = strsep(&copy, ")}");                      // meta cycles
            
        strtrim(code); strtrim(desc); strtrim(cycle);     // remove whitespace
        desc = tolowerstr(desc);                          // descriptor is case insensitive
        
        if (isempty(token) || *token==' ')  continue;     // Empty meta-data line 
         
        if (streq(desc, "start")) desc = strdup("begin"); // start == begin
        if (streq(desc, "end")) desc = strdup("finish");  // end == finish             
        
        codeVdesc(token, code, desc);                     // Validate code and descriptor 
        int c = verifycycle(code, desc, cycle);           // Validate cycle value
        int mstot = getmstot(c, desc, conf);              // total msec for operation  
        
        pcb.burstms = pcb.burstms + mstot;                // burst time of process
                                                          
        struct Instruction inst;                          // current meta-data instruction
                                                          
        strcpy(inst.code, code);                          // instruction code e.g. S, A, I, O
        strcpy(inst.desc, desc);                          // descriptor e.g. begin, printer, allocate
        inst.c = c;                                       // instruction cycles
        inst.mstot = mstot;                               // instruction total cycle time in ms
                                                          
        pcb.inst[pcb.inum] = inst;                        // store instruction in PCB instruction array
        pcb.inum++;                                       // instruction count of process
        
        if (*code == 'S'){           
            pcb.id = 0;
            qput(pcb);
            pcb = emptypcb;            
        } else if ( *code == 'A' && streq(desc, "finish")) {
                pcb.id = procid;
                qput(pcb);                              
                
                procid = procid + 1;
                pcb = emptypcb;           
        }
        //_log("\n%d, %s{%s}%s - %d ms\n", icount, code, desc, cycle, mstot); // Debug
    }
    
    
    
    // Simulate Meta-file from ready queue
    
    unsigned int memblocktail = 0;
    bool oson=0;
    inproc=0;
    
    while (!qisempty()){
        pcb = qget();
        
        int id = pcb.id;
        icount = 0;
        
        
        //_log("\nProcess %d, Instructions: %d\n", pcb.id, pcb.inum); // debug
        
        for (icount = 0; icount < pcb.inum; icount++){
            struct Instruction it = pcb.inst[icount];
            
            char *code = strdup(it.code);
            char *desc = strdup(it.desc);
            int c = it.c;
            int mstot = it.mstot;
            
           // _log("\n-- %d, %s{%s}%d - %d\n", icount, code, desc, c, mstot); //debug
            
            if ( *code == 'S'){
                if ( streq(desc, "begin") ){
                    if (oson==1) logerror("\nError -- OS is already on, Invalid instruction: %s{%s}%c", code, desc, c);
                    oson ++;
                    _log("%.6f - Simulator program starting\n", now());
                }
                else if ( streq(desc, "finish")){
                    if (inproc==1) logerror("\nError -- OS turned off during running process");
                    if (oson==0) logerror("\nError -- OS is already off, Invalid instruction: %s{%s}%c", code, desc, c);
                    oson--;
                    _log("%.6f - Simulator Program ending\n", now());
                }
            } else if (oson == 0) logerror("\nError -- OS is turned off, Invalid instruction: %s{%s}%c", code, desc, c);
            
            if ( *code == 'A' ){
                if ( streq(desc, "begin") ){
                    inproc++;
                    _log("%.6f - OS: preparing process %d\n", now(), id);
                    pcb.state = START;
                    _log("%.6f - OS: starting process %d\n", now(), id);
                    pcb.state = READY;
                } else if ( streq(desc, "finish")) {
                    inproc--;
                    _log("%.6f - OS: removing process %d\n", now(), id);
                    pcb.state = EXIT;
                }
            } else if ( *code == 'P' ){
                pcb.state = RUNNING;
                _log("%.6f - Process %d: start processing action\n", now(), id);
                wait(mstot);
                _log("%.6f - Process %d: end processing action\n", now(), id);
            } else if ( *code == 'I' || *code == 'O' ){
                struct Io_d io1 = {mstot, id, code, desc};
                pcb.state = WAITING;
                io_thread(&io1);
                pcb.state = RUNNING;
            } else if (*code == 'M'){
                if (streq(desc, "block")){
                    _log("%.6f - Process %d: start memory blocking\n", now(), id);
                    wait(mstot);
                    _log("%.6f - Process %d: end memory blocking\n", now(), id);
                } else if (streq(desc, "allocate")){
                    if (conf.memkb == 0) logerror("Error -- system memory qsize not given in config file");
                    if (conf.memblock == 0) logerror("Error -- memory block qsize not given in config file");
                    
                    _log("%.6f - Process %d: allocating memory\n", now(), id);
                    
                    unsigned int loc = conf.memblock*memblocktail++;
                    if (memblocktail >= conf.memkb / conf.memblock)
                        memblocktail = 0;
                    
                    wait(mstot);
                    _log("%.6f - Process %d: memory allocated at 0x%08X\n", now(), id, loc);
                }
            }
            
        }
    }
    
    if (!validend) {logerror("\nError -- 'End Program Meta-Data Code:' not found\n");}
}



// ================= PCB Ready Queue Functions ================
struct PCB qpeek() { return readyq[front]; }

bool qisempty() { return inq == 0; }

bool qisfull() { return inq == QMAX; }

int qsize() { return inq; }  

void qput(struct PCB data) {
   if(!qisfull()) {
	
      if(rear == QMAX-1) {rear = -1;}       

      readyq[++rear] = data;
      inq++;
   }
}

struct PCB qget() {
   struct PCB data = readyq[front++];
	
   if(front == QMAX) {front = 0;}
	
   inq--;
   return data;  
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
    
    struct Io_d io = *(struct Io_d*) arg;
    int msec = io.msec, procid = io.procid;
    char *code = io.code, *desc = io.desc, *temp;
    
    if (streq(code, "I")) temp = strdup("input");
    else if (streq(code, "O")) temp = strdup("output");
    
    char *part;
    int *lock, *cur, curmax=-1;
    if (streq(desc, "keyboard")) lock = &keybmut;
    else if (streq(desc, "mouse")) lock = &mousemut;
    else if (streq(desc, "monitor")) lock = &monmut;
    else if (streq(desc, "hard drive")) {
        lock = &hddsem;
        cur = &curhdd;
        curmax = conf.hdd_q - 1;
        part = strdup("HDD");
    } else if (streq(desc, "printer")) {
        lock = &printsem;
        cur = &curprint;
        curmax = conf.printer_q - 1;
        part = strdup("PRIN");
    }
    
    int maxretry = 1000;
    int retry = 0;
    
    while(1){
        int unlocked = *lock;
        if (unlocked) {
            *lock = *lock-1;
            break;
        }
        
        wait(1);
        
        retry ++;
        if (retry == maxretry)
            logerror("\nError in simulation -- Cannot lock resource %s after %d retries\n", desc, maxretry);
    }    
    
    if (curmax > -1) {
        _log("%.6f - Process %d: start %s %s on %s %d\n", now(), procid, desc, temp, part, *cur);
        *cur = *cur + 1;
        if (*cur > curmax) *cur = 0;
    } else
        _log("%.6f - Process %d: start %s %s\n", now(), procid, desc, temp);
   
    wait(msec);
    _log("%.6f - Process %d: end %s %s\n", now(), procid, desc, temp);

    *lock = *lock+1;
}


void io_thread(struct Io_d *io_ptr){
    // create/join I/O thread, log and wait
    
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, wait_t, io_ptr); 
    pthread_join(thread_id, NULL); 
    return;
}





// ============= Meta-Data File Related Functions ==============

char *file2str(FILE *f){
    // Read meta file into buffer string
    char *buffer = 0;            
    
    fseek (f, 0, SEEK_END);
    long length = ftell(f);
    fseek (f, 0, SEEK_SET);
    
    buffer = (char *) malloc(length);
    fread (buffer, 1, length, f);
    if (!buffer) logerror("\nError -- reading meta-data file\n");
    
    return buffer;
}


char *startmeta(char *buffer){
    // Detect 'Start Program Meta-Data Code:', raise Error if absent
    char *token;
    while ((token = strsep(&buffer, ":;.")) != NULL) {
        strtrim(tolowerstr(token));
        if (streq(token, "start program meta-data code")) return buffer;
    }
    logerror("\nError -- Start Program Meta-Data Code: not found\n");
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


int getmstot(int c, const char *desc, struct Config conf){
    // Calculate total msec for operation
    
    int mspc;            // millisesconds per cycle
    
    if (streq(desc, "hard drive"))    mspc = conf.hdd;
    else if (streq(desc, "keyboard")) mspc = conf.keyboard;
    else if (streq(desc, "mouse"))    mspc = conf.mouse;
    else if (streq(desc, "monitor"))  mspc = conf.monitor;
    else if (streq(desc, "run"))      mspc = conf.processor;
    else if (streq(desc, "allocate")) mspc = conf.memory;
    else if (streq(desc, "printer"))  mspc = conf.printer;
    else if (streq(desc, "block"))    mspc = conf.memory;
    else if (streq(desc, "cache"))    mspc = conf.memory;
    else mspc = 0;
    
    if (mspc == 0 && !streq(desc, "begin") && !streq(desc, "finish"))
        logerror("\nError -- '%s' cycle time not given in config file\n", desc);
    
    return c*mspc;    // Total time for operation
}





// ============= Configuration File Related Functions =============

void startconf(FILE *fconf){
    // Detect 'Start Simulator Configuration File'; raise Error if absent
    
    char line[256];    

    while (fgets(line, sizeof(line), fconf)){
        strtrim(line);                  // remove leading/trailing/multiple whitespace
        tolowerstr(line);               // config file is case insensitive
        
        if (streq(line, "start simulator configuration file")) 
            return;                     // start of config file detected
    }
    // 'Start Simulator Configuration File' not detected
    logerror("\nError in config file -- 'Start Simulator Configuration File' not found");
}


int verify(char *val, const char *part, const char *msg){
    // validate cycle msec or memory kbytes as positive number
    int m = 1;
    char *unit=0;
    
    if ( !streq(msg, "quantity")) {
        unit = strsep(&val, "})");      // unit = msec for cycle, kbytes/Mbytes/Gbytes for memory
        strsep(&val, ":");
        strtrim(unit); strtrim(val);    // remove whitespace
    }
        
    if (streq(msg, "qsize")){       
        if (streq(unit, "kbytes")) ;
        else if (streq(unit, "mbytes")) m = 1024;
        else if (streq(unit, "gbytes")) m = 1024*1024;
        else logerror("\nError -- Invalid unit for '%s' %s: %s", part, msg, unit);
    } 
    else if (!streq(msg, "quantity") && !streq(unit, "msec"))
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


struct Config getconfig(FILE *fconf){
    // Gets config data into struct for future use
    
    startconf(fconf);                   // Detect 'Start Simulator Configuration File' or raise error
    
    bool metafilegiven = 0;             // true if name of meta file given
    bool endconf = 0;                   // true if config file terminates normally
    char line[256];                            
    
    while (fgets(line, sizeof(line), fconf)){
        strtrim(line);                  // Remove leading/trailing/multiple whitespace
        
        char *copy = &line[0];
        char *key = strsep(&copy, "{(:"); // key = before :
        char *val = copy;               // value = after :
        char *vcpy = strdup(val);
        
        strtrim(key); 
        strtrim(val);                   // Remove leading/trailing/multiple whitespace
        strtrim(vcpy);
        tolowerstr(key);
        tolowerstr(val);

        if (isempty(line)) continue;    // Empty line
        
        if(streq(key, "end simulator configuration file")){         // end of conf file
            endconf = 1; 
            break;
        } else if (isempty(val)){ logerror("\nError in config file -- missing value for %s", key);
        } else if (streq(key, "monitor display time")){ conf.monitor = verify(val, "monitor", "display time");
        } else if (streq(key, "processor cycle time")){ conf.processor = verify(val, "processor", "cycle time"); 
        } else if (streq(key, "mouse cycle time")){     conf.mouse = verify(val, "mouse", "cycle time");
        } else if (streq(key, "hard drive cycle time")){conf.hdd = verify(val, "hard drive", "cycle time");
        } else if (streq(key, "keyboard cycle time")){  conf.keyboard = verify(val, "keyboard", "cycle time");
        } else if (streq(key, "memory cycle time")){    conf.memory = verify(val, "memory", "cycle time");
        } else if (streq(key, "printer cycle time")){   conf.printer = verify(val, "printer", "cycle time");
        } else if (streq(key, "system memory")){        conf.memkb = verify(val, "system memory", "qsize");
        } else if (streq(key, "memory block size")){    conf.memblock = verify(val, "memory block", "qsize");
        } else if (streq(key, "version/phase")){        strcpy(conf.version, vcpy);
        } else if (streq(key, "log file path")){        strcpy(conf.log_fname, log_fname);
        } else if (streq(key, "file path")){            strcpy(conf.meta_fname, vcpy);
		} else if (streq(key, "hard drive quantity")){  conf.hdd_q = verify(val, "hard drive", "quantity");
                                                        hddsem = conf.hdd_q;
		} else if (streq(key, "printer quantity")){     conf.printer_q = verify(val, "printer", "quantity");
                                                        printsem = conf.printer_q;
        } else if (streq(key, "log")){
            if (log2mon && !log2file)      sprintf(conf.logmode, "%s", "monitor");
            else if (!log2mon && log2file) sprintf(conf.logmode, "%s", log_fname);
            else if (log2mon && log2file)  sprintf(conf.logmode, "monitor and %s", log_fname);
        } else logerror("\nError in config file -- Invalid config line: %s\n", line);
    }

    if (streq(conf.meta_fname, "None")) // Meta file name not given
        logerror("\nError in config file -- no meta file specified!\n"); 
    
    if (!endconf)                       // Config file abruptly terminated
        logerror("\nError - config file abruptly terminated -- 'End Simulator Configuration File' not found!\n"); 
    
    return conf;
}



// ============= String Functions =============

int    _atoi(const char *str) {
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


bool isempty(const char *str) {return (str == NULL || str[0] == 0);}


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



// ============== I/O Functions ==============

int _log(const char *format, ...){
    // log to monitor/file/both based on config, wrapper on fprintf, replaces printf & fprintf
    int r;
    va_list arg1, arg2;
    va_start(arg1, format);
	va_copy(arg2, arg1);
	if (log2file) { r = vfprintf(logfile, format, arg1); }      // global logfile
    if (log2mon) { r = vfprintf(stdout, format, arg2); }        // stdout = monitor
    
    va_end(arg1);
    return r;
}


void logerror(const char *format, ...){
    // log error message and exit
    
    va_list arg;
    va_start(arg, format);
    if (log2mon) { vfprintf(stdout, format, arg); }
    if (log2file) { vfprintf(logfile, format, arg); }
    va_end(arg);
    exit(0);
}


FILE *fopenval(const char* fname, struct Fin fin){
    // Open & validate config or meta file
    
    if (isempty(fname))
        logerror("\nError -- missing '%s' filename\n", fin.type);
        
    char* ext = strchr(fname, '.');
    if ( !streq(ext, fin.ext) )          // Validate Extension 
        logerror("\nError -- Invalid %s file extension: %s\n", fin.type, ext);
    
    FILE* f = fopen(fname, "r");         // Open file 
    
    if (f == NULL)                       // Validate file exists 
        logerror("\nError -- cannot open %s file, invalid filename: %s\n", fin.type, fname);
    
    fseek (f, 0, SEEK_END);
    long qsize = ftell(f);                // File qsize
    
    if (0 == qsize)                       // Validate not empty file
        logerror("\nError -- %s file is empty\n", fin.type);
    
    fseek(f, 0, SEEK_SET);               // Reset file pointer to read again
    return f;            
}


void setlogmode(FILE *fconf){
    // Sets logger to monitor/file/both using global flags log2mon, log2file
    // based on config file to be used by functions _log() and logerror() 
    
    bool validlogmode = 0,              // flag - true if log mode given in config file
         validlogfile = 0;              // flag - true if log filepath given
         
    char line[256];
    
    while (fgets(line, sizeof(line), fconf)){
        strtrim(line);                  // Remove leading/trailing/multiple whitespace
        //tolowerstr(line);               // Config file is case insensitive
        
        char *temp = &line[0];        
        char *key = strsep(&temp, ":"); // key = before :
        char *val = temp;               // value = after :
        char *vcpy = strdup(val);
        
        strtrim(key); 
        strtrim(val);                   // Remove leading/trailing/multiple whitespace
        strtrim(vcpy);
        tolowerstr(key);
        tolowerstr(val);

        if (isempty(line)) continue;    // Empty line

        if (streq(key, "log")){    
            if (streq(val, "log to file"))         { log2mon = 0; log2file = 1;}
            else if (streq(val, "log to monitor")) { log2mon = 1; log2file = 0;}
            else if (streq(val, "log to both"))    { log2mon = 1; log2file = 1;}
            else {logerror("\nError -- Invalid logging mode: %s\n", val);}
            validlogmode = 1;
        } 
        else if( streq(key, "log file path") && !isempty(val)){
            strcpy(log_fname, vcpy);
            logfile = fopen(log_fname, "w");
            validlogfile = 1;
        }
    }
    
    // Exit program without logging anything if _log mode not specified 
    if (!validlogmode) 
        logerror("\nError -- No logging mode specified in config file\n"); 
    
    // Log file path is required only if log to file or log to both 
    if (log2file && (!validlogfile)){
        printf("\nError -- Invalid or no logfile specified\n");
        exit(0);
    }
    fseek(fconf, 0, SEEK_SET);          // Reset config file pointer for getconfig(FILE *)

}


