// ================= Instruction =====================
struct instruction { char text, struct instruction *next;}

// ================= PCB Ready Queue =====================
enum state {START, READY, RUNNING, WAITING, EXIT};// pcb states

struct PCB {int id, burstms; enum state state; struct instruction *next};    // struct to store simulator pcb state

#define MAX 30

static struct PCB readyq[MAX];
static int front = 0;
static int rear = -1;
static int inq = 0;

struct PCB peek() { return readyq[front]; }

bool isEmpty() { return inq == 0; }

bool isFull() { return inq == MAX; }

int size() { return inq; }  

void put(struct PCB data) {
   if(!isFull()) {
	
      if(rear == MAX-1) {rear = -1;}       

      readyq[++rear] = data;
      inq++;
   }
}

struct PCB get() {
   struct PCB data = readyq[front++];
	
   if(front == MAX) {front = 0;}
	
   inq--;
   return data;  
}


char *startmeta(char *buffer){
    // Detect 'Start Program Meta-Data Code:' 
    // Detect 'End Program Meta-data Code.'
    // raise Error if absent
    
    bool validstart = 0, validend = 0;
    
    char *token;
   
    while ((token = strsep(&buffer, ":;.")) != NULL) {
        strtrim(tolowerstr(token));
        if (streq(token, "start program meta-data code")) {
            validstart = 1;
            break;
        }
    }
    
    logerror("\nError -- Start Program Meta-Data Code: not found\n");
}


// Read Meta file into PCB Queue

void readmeta(FILE *fmeta, struct Config conf){
    char *buffer = file2str(fmeta);
    buffer = startmeta(buffer);
    
    char *token, *copy, *code, *desc, *cycle;
    
    while ((token = strsep(&buffer, ":;.")) != NULL) {
        if (streq(copy, "end program meta-data code")) return;
        
        
    }
}



void simulator(FILE *fmeta, struct Config conf){
    // Simulates meta-file
    struct PCB pcbarr[20];
    
    char *buffer = file2str(fmeta);    // Read meta file into buffer string    
    buffer = startmeta(buffer);        // Detect Start Program Meta-Data Code: 
    char *token, *copy;                // token = meta-data code e.g. A{begin}0
    char *code, *desc, *cycle;         // meta-data code, descriptor, cycles
    
    int proccount = 0;                 // process count - 1,2 ...
    
    unsigned int memblocktail = 0;
    
    while ((token = strsep(&buffer, ":;.")) != NULL) {
        strtrim(token);                                   // remove whitespace
        copy = strdup(token);                             // copy of token
        tolowerstr(copy);                                 // End program meta-data is case-insensitive

        if (streq(copy, "end program meta-data code"))    // end of meta file
            return;
        
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

        
        //_log("\n%s{%s}%s - %d ms\n", code, desc, cycle, mstot); // Debug
        
        if ( *code == 'S'){
            if ( streq(desc, "begin") ){
                _log("%.6f - Simulator program starting\n", now());
            }
            else if ( streq(desc, "finish")){
                _log("%.6f - Simulator Program ending\n", now());
            }
        } else if ( *code == 'A' ){
            if ( streq(desc, "begin") ){
                proccount++;
                _log("%.6f - OS: preparing process %d\n", now(), proccount);
                pcbarr[proccount].state = START;
                _log("%.6f - OS: starting process %d\n", now(), proccount);
                pcbarr[proccount].state = READY;
            } else if ( streq(desc, "finish")) {
                _log("%.6f - OS: removing process %d\n", now(), proccount);
                pcbarr[proccount].state = EXIT;
            }
        } else if ( *code == 'P' ){
            pcbarr[proccount].state = RUNNING;
            _log("%.6f - Process %d: start processing action\n", now(), proccount);
            wait(mstot);
            _log("%.6f - Process %d: end processing action\n", now(), proccount);
        } else if ( *code == 'I' || *code == 'O' ){
            struct Io_d io1 = {mstot, proccount, code, desc};
            pcbarr[proccount].state = WAITING;
            io_thread(&io1);
            pcbarr[proccount].state = RUNNING;
        } else if (*code == 'M'){
            if (streq(desc, "block")){
                _log("%.6f - Process %d: start memory blocking\n", now(), proccount);
                wait(mstot);
                _log("%.6f - Process %d: end memory blocking\n", now(), proccount);
            } else if (streq(desc, "allocate")){
                if (conf.memkb == 0) logerror("Error -- system memory size not given in config file");
                if (conf.memblock == 0) logerror("Error -- memory block size not given in config file");
                
                _log("%.6f - Process %d: allocating memory\n", now(), proccount);
                
                unsigned int loc = conf.memblock*memblocktail++;
                if (memblocktail >= conf.memkb / conf.memblock)
                    memblocktail = 0;
                
                wait(mstot);
                _log("%.6f - Process %d: memory allocated at 0x%08X\n", now(), proccount, loc);
            }
        }
        
        //_log("Program state: %d\n", pcbarr[proccount].state);        // Debug
    }
    
    logerror("\nError -- 'End Program Meta-Data Code:' not found\n");

}










