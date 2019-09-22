/**
* sim1.c
*
* Project 1
*
* CS 646 Fall 2019
* 
* To compile, run makefile by entering "make"
*
* To run ./sim1 sim1.conf
*
* Farhan Sadique
*/


#include <stdarg.h> 							// for va_start, va_end
#include <stdio.h>								// for printf, FILE, fopen etc.
#include <stdlib.h>								// for exit(), malloc(), atoi()

// Typdef boolean
typedef enum { false, true } bool;

/* Global Variables */
bool log2mon = 1, log2file = 0; 				// Flag - log to monitor, file
char log_fname[50];								// log file name
FILE *logfile;									// log file 
                                            
/* Structure definitions */                 
struct Fin{char type[10], ext[10];};			// Used to validate config/meta file 

struct Config {									// Configuration 
	int monitor, processor, mouse, hdd, keyboard, memory, printer;
	char version[10], meta_fname[50], logmode[20], log_fname[50]; 
};
                                            
/* Function declarations */  
bool streq(const char *str1, const char *str2); // returns true if str1 == str2, false otherwise               	
int _log(const char *format, ...);				// logs to monitor/file/both based on config
void logerror(const char *format, ...);			// log error message and exit
int isspace(int c);								// true if c is whitespace (' ',\t,\n,\r)
void strtrim(char *str);						// removes leading/trailing/multiple whitespace
char *tolowerstr(char str[]);               	// makes str lowercase
char *strcpy(char *dest, const char *src);		// Copy src to dest
int _strlen(const char *str);					// Length of str in bytes
char *strdup(const char *str);					// Copy of str with memory allocation
char *strchr(const char *s, int c);				// points to 1st occurence of c in s
char *strsep(char **stringp,const char *delim);	// slice up stringp at delim
bool isempty(char *str);						// true if str is empty
FILE *fopenval(const char* fname, struct Fin fin); // opens & validates config/meta file
void setlogmode(FILE *fconf); 					// sets logger to monitor/file/both  
int verify_n_log(char* val, const char *part); 	// logs if cycle ms exists & nonnegative
struct Config logconfig(FILE *fconf);			// Logs config data from config file
void logmeta(FILE *fmeta, struct Config conf);	// Logs meta data from meta file


// main function 
int main(int argc, char* argv[]) {	
	struct Fin fin_conf = {"config", ".conf"}, 	// config file must have .conf ext
	fin_meta = {"meta", ".mdf"};				// meta file must have .mdf extension
	
	// Config file 
    const char* config_fname = argv[1]; 		// Config filename
	FILE* fconf;								// Config file
	fconf = fopenval(config_fname,fin_conf);	// Open Config file
	
	setlogmode(fconf);							// set log mode to monitor/file/both
	
	struct Config conf = logconfig(fconf);		// log config and store in struct conf
	
	fclose(fconf);								// Close config file

	// Meta file 
	FILE * fmeta;								// Meta file
	fmeta = fopenval(conf.meta_fname, fin_meta);// Open Meta file
	
	logmeta(fmeta, conf);						// log meta-data to monitor/file/both
	
	fclose(fmeta);								// Close Meta file
	
	// Close Log file if open
	if (logfile != NULL) fclose(logfile); 						
	
    return 1;
}


// Other Functions 

bool streq(const char *s1, const char *s2) {
	if (s1 == NULL) { return 0; }
   while (*s1 != '\0' && *s2 != '\0'  && *s1 == *s2) {
      s1++;
      s2++;
   }
   return (*s1 - *s2) == 0;
}


int _log(const char *format, ...){
	/* logs to monitor/file/both based on config
	* Wrapper around fprintf to be used by all functions  
	* instead of printf or fprintf */
	int r;
	va_list arg;
	va_start(arg, format);
	if (log2mon) { r = vfprintf(stdout, format, arg); }		// stdout = monitor
	if (log2file) { r = vfprintf(logfile, format, arg); }	// global logfile
	va_end(arg);
	return r;
}


void logerror(const char *format, ...){
	// _log error message and exit
	va_list arg;
	va_start(arg, format);
	if (log2mon) { vfprintf(stdout, format, arg); }
	if (log2file) { vfprintf(logfile, format, arg); }
	va_end(arg);
	exit(0);
}


int isspace(int c) {return ((c>=0x09 && c<=0x0D) || (c==0x20));}


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


char *strcpy(char *dest, const char *src) {
   char *save = dest;
   while(*dest++ = *src++);
   return save;
}


int _strlen(const char *str){
  if (!str) return 0;
  const char *ptr = str;
  while (*str) ++str;
  return str - ptr;
}


char *strdup(const char *str) {
    if (str == NULL) return NULL;
    char *copy = malloc(_strlen(str) + 1);
    if (copy != NULL)
        return strcpy(copy, str);
}


char *strchr(const char *s, int c) {
    while (s[0] != c && s[0] != '\0') s++;
    if (s[0] == '\0')  return NULL;
    return (char *)s;
}


char *strsep(char **stringp, const char *delim) {
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


bool isempty(char *str) {return (str == NULL || str[0] == 0);}


FILE *fopenval(const char* fname, struct Fin fin){
	// Open & validate config or meta file
	
	char* ext = strchr(fname, '.');
	if ( !streq(ext, fin.ext) )			// Validate Extension 
		logerror("Error -- Invalid %s file extension: %s", fin.type, ext);
	
	FILE* f = fopen(fname, "r");		// Open file 
	
	if (f == NULL)						// Validate file exists 
		logerror("Error -- cannot open %s file, invalid filename: %s", fin.type, fname);
	
	fseek (f, 0, SEEK_END);
	long size = ftell(f);				// File size
	
	if (0 == size) 						// Validate not empty file
		logerror("Error -- %s file is empty\n", fin.type);
	
	fseek(f, 0, SEEK_SET);				// Reset file pointer to read again
	return f;			
}


void setlogmode(FILE *fconf){
	// Sets logger to monitor/file/both using global flags log2mon, log2file
	// based on config file to be used by functions _log() and logerror() 
	
	bool validlogmode = 0, validlogfile = 0;
	char line[256];
	while (fgets(line, sizeof(line), fconf)){
		strtrim(line);						// Remove leading/trailing/multiple whitespace
		tolowerstr(line);					// Config file is case insensitive
		
		char *temp = &line[0];		
		char *key = strsep(&temp, ":");		// key = before :
		char *val = temp;					// value = after :
		strtrim(key); strtrim(val);			// Remove leading/trailing/multiple whitespace

		if (isempty(line)) continue;		// Empty line

		if (streq(key, "log") && !isempty(line)){	
			if (streq(val, "log to file"))			{ log2mon = 0; log2file = 1;}
			else if (streq(val, "log to monitor"))	{ log2mon = 1; log2file = 0;}
			else if (streq(val, "log to both"))		{ log2mon = 1; log2file = 1;}
			else {logerror("Error -- Invalid logging mode: %s", val);}
			validlogmode = 1;
		} 
		else if( streq(key, "log file path") && !isempty(line)){
			strcpy(log_fname, val);
			logfile = fopen(log_fname, "w");
			validlogfile = 1;
		}
	}
	
	// Exit program without logging anything if _log mode not specified 
	if (!validlogmode) 
		logerror("Error -- No logging mode specified in config file"); 
	
	// Log file path is required only if log to file or log to both 
	if (log2file && (!validlogfile)){
		printf("Error -- Invalid or no logfile specified");
		exit(0);
	}
	fseek(fconf, 0, SEEK_SET);	// Reset config file pointer for logconfig(FILE *)

}


int verify_n_log(char* val, const char *part){
	// checks if cycle ms exists & nonnegative, part = processor, memory etc.
	
	if (val == NULL || val[0] == 0) 	// Missing cycle ms
		logerror("\nError -- '%s' cycle time is missing\n", part);
	int v = atoi(val);
	if (v < 0)  						// Negative cycle ms
		logerror("\nError -- Invalid '%s' cycle time: %d\n", part, v);
	_log("%s = %d ms/cycle\n", part, v); 
	return v;							// ms stored in config struct
}


struct Config logconfig(FILE *fconf){
	// Logs config data to monitor/file/both
	
	bool metafilegiven = 0;
	struct Config conf = {0,0,0,0,0,0,0,"None","None","None","None"};;
	char line[256];							
	
	_log("Configuration File Data\n");
	
	bool startconf = 0;						// true if start simulator configuration present
	
	// Detect 'Start Simulator Configuration File'
	while (fgets(line, sizeof(line), fconf)){
		strtrim(line);						// Remove leading/trailing/multiple whitespace
		tolowerstr(line);					// Config file is case insensitive
		
		if (streq(line, "start simulator configuration file")){
			startconf = 1;
			break;
		}
    }
	 
	// Raise error if 'Start Simulator Configuration File' absent 
	if (!startconf) 
		logerror("\nError -- 'Start Simulator Configuration File' not found");
	
	// Read configuration
	bool endconf = 0;
	
	while (fgets(line, sizeof(line), fconf)){
		strtrim(line);						// Remove leading/trailing/multiple whitespace
		tolowerstr(line);					// Config file is case insensitive
		
		char *temp = &line[0];		
		char *key = strsep(&temp, ":");		// key = before :
		char *val = temp;					// value = after :
		strtrim(key); strtrim(val);			// Remove leading/trailing/multiple whitespace

		if (isempty(line)) continue;		// Empty line
		
		if (streq(key, "monitor display time {msec}")){
			conf.monitor = verify_n_log(val, "Monitor");
		} else if(streq(key, "processor cycle time {msec}")){
			conf.processor = verify_n_log(val, "Processor"); 
		} else if(streq(key, "mouse cycle time {msec}")){
			conf.mouse = verify_n_log(val, "Mouse");
		} else if(streq(key, "hard drive cycle time {msec}")){
			conf.hdd = verify_n_log(val, "Hard Drive");
		} else if(streq(key, "keyboard cycle time {msec}")){
			conf.keyboard = verify_n_log(val, "Keyboard");
		} else if(streq(key, "memory cycle time {msec}")){
			conf.memory = verify_n_log(val, "Memory");
		} else if(streq(key, "printer cycle time {msec}")){
			conf.printer = verify_n_log(val, "Printer");
		} else if(streq(key, "version/phase")){
			strcpy(conf.version, val);
		} else if(streq(key, "file path")){
			if (val == NULL || val[0] == 0){ 
				logerror("Error -- no meta file specified!\n"); 
			} else { 
				strcpy(conf.meta_fname, val);
				metafilegiven = 1;
			}
		} else if(streq(key, "log")){
			if (log2mon && !log2file) 		sprintf(conf.logmode, "%s", "monitor");
			else if (!log2mon && log2file)	sprintf(conf.logmode, "%s", log_fname);
			else if (log2mon && log2file)	sprintf(conf.logmode, "monitor and %s", log_fname);
			_log("Logged to: %s\n\n", conf.logmode);
		} else if(streq(key, "log file path")){
			strcpy(conf.log_fname, log_fname);
		} else if(streq(key, "end simulator configuration file")){ 		// end of conf file
			endconf = 1;
			break;
		} else 
			logerror("Error -- Invalid config line: %s\n", line);
		}
	
	if (!metafilegiven){ logerror("\nError -- no meta file specified!"); }
	if (!endconf) logerror("\nError -- 'End Simulator Configuration File' not found!"); 
	
	return conf;
}


void logmeta(FILE *fmeta, struct Config conf){
	// Logs meta data from meta file
	
	// Read meta file into buffer string
	char *buffer = 0;			
	long length;
	
	fseek (fmeta, 0, SEEK_END);
	length = ftell(fmeta);
	fseek (fmeta, 0, SEEK_SET);
	buffer = (char *) malloc(length);
	if (!buffer) logerror("Error -- reading meta-data file");
	fread (buffer, 1, length, fmeta);
	
	// Detect Start Program Meta-Data Code: 
	char *token, *copy;
	bool startmeta = 0;			// true if start meta-data present
	
	while ((token = strsep(&buffer, ":;.")) != NULL) {
		strtrim(token);
		copy = strdup(token);
		tolowerstr(copy);
		
		if (streq(copy, "start program meta-data code")){
			startmeta = 1;
			break;
		}
    }
	// Raise error if Start Program Meta-Data Code: absent 
	if (!startmeta) 
		logerror("Error -- Start Program Meta-Data Code: not found\n");
	
	
	// Validate and log each meta data 
	_log("Meta-Data Metrics\n");
	
	char *code, *desc, *cycle;	// meta-data code, descriptor, cycles
	bool endmeta = 0;			// true meta-file terminates normally 
	
	while ((token = strsep(&buffer, ":;.")) != NULL) {
		strtrim(token);
		copy = strdup(token);
		tolowerstr(copy);

		if (streq(copy, "end program meta-data code")){	// End of meta file
			endmeta = 1;
			break;
		}

		copy = strdup(token);

		code = strsep(&copy, "{(");		// meta code		
		desc = strsep(&copy, "})");		// meta descriptor
		cycle = strsep(&copy, ")}");	// meta cycles
			
		strtrim(code); strtrim(desc); strtrim(cycle);	// remove whitespace
		desc = tolowerstr(desc);						// descriptor is case insensitive

		
		// Empty meta-data line 
		if (isempty(code)) continue;
		if (streq(code, " ") && (desc==NULL) && (cycle==NULL))
			continue;
		
		// start == begin, end == finish 
		if (streq(desc, "start")) desc = strdup("begin");
		if (streq(desc, "end")) desc = strdup("finish");
		
		//_log("%s %s %s %s\n", token, code, desc, cycle);		// Debug
								
		
		// Validate Code and descriptor 
		char SA[][20] = {"begin", "finish"};		
		char P[][20]  = {"run"};
		char I[][20]  = {"hard drive", "keyboard", "mouse"};
		char O[][20]  = {"hard drive", "printer", "monitor"};
		char M[][20]  = {"block", "allocate"};
		
		char (*d)[20];
		if (*code == 'S' || *code == 'A') d = SA;
		else if (*code == 'P') d = P;
		else if (*code == 'I') d = I;
		else if (*code == 'O') d = O;
		else if (*code == 'M') d = M;
		else logerror("\nInvalid meta-data code %s in meta-data %s", code, token);
		
		bool validdesc = 0;
		for (int i=0; i<10; i++)
			if (streq(desc, d[i]))
				validdesc = 1;
		
		if (!validdesc)
			logerror("\nError -- Invalid descriptor '%s', for code %s, in meta-data '%s'\n", desc, code, token);

		
		// Validate cycles
		bool invalid_cycle = 0;
		if (isempty(cycle)) logerror("\nError -- missing cycle value for '%s' in: '%s{%s}%s'", desc, code, desc, cycle);
		
		for (int i=0; i < _strlen(cycle); i++)
			if ( (cycle[i] < '0' || cycle[i] >'9') && cycle[i] != '+')
				invalid_cycle = 1;
		
		int c = atoi(cycle);
		
		if (c == 0 && !streq(desc, "begin") && !streq(desc, "finish")) invalid_cycle = 1;
		
		if (invalid_cycle) 
			logerror("\nError -- invalid cycles '%s' for '%s' in: '%s{%s}%s'", cycle, desc, code, desc, cycle);
		
		
		// Calculate total time 
		int mspc;			// millisesconds per cycle
		
		if (streq(desc, "hard drive"))		mspc = conf.hdd;
		else if (streq(desc, "keyboard"))	mspc = conf.keyboard;
		else if (streq(desc, "mouse"))		mspc = conf.mouse;
		else if (streq(desc, "monitor"))	mspc = conf.monitor;
		else if (streq(desc, "run"))		mspc = conf.processor;
		else if (streq(desc, "allocate"))	mspc = conf.memory;
		else if (streq(desc, "printer"))	mspc = conf.printer;
		else if (streq(desc, "block"))		mspc = conf.memory;
		else mspc = 0;
		
		if (mspc == 0 && !streq(desc, "begin") && !streq(desc, "finish"))
			logerror("\nError -- '%s' cycle time not given in config file", desc);
		
		int mstot = c*mspc;	// Total time for operation
		
		if ( !streq(desc, "begin") && !streq(desc, "finish"))
			_log("%s{%s}%s - %d ms\n", code, desc, cycle, mstot);
    }
	
	if (!endmeta) 
		logerror("\nError -- 'End Program Meta-Data Code:' not found");

}
