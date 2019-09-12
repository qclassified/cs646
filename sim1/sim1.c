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

#include <ctype.h>
#include <stdarg.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int log(const char *format, ...);				// logs to monitor/file/both based on config
void logerror(const char *format, ...);			// log error message and exit
int verify_n_log(char* val, const char *part); 	// logs if cycle ms exists & nonnegative
char *tolowerstr(char str[]);               	// makes str lowercase
void strtrim(char *str);						// removes whitespace 
void setlogmode(FILE *fconf); 					// sets logger to monitor/file/both 
FILE *fopenval(const char* fname, Fin fin);		// opens & validates config/meta file 
struct Config logconfig(FILE *fconf);			// Logs config data from config file
void logmeta(FILE *fmeta, struct Config conf);	// Logs meta data from meta file


/* main function */
int main(int argc, char* argv[]) {	
	struct Fin fin_conf = {"config", ".conf"}, 	// config file must have .conf ext
	fin_meta = {"meta", ".mdf"};				// meta file must have .mdf extension
	
	/* Config file */
    const char* config_fname = argv[1]; 			// Config filename
	FILE* fconf = fopenval(config_fname,fin_conf);	// Config file
	
	setlogmode(fconf);							// set log mode to monitor/file/both
	struct Config conf = logconfig(fconf);		// log config and store in struct conf
	
	fclose(fconf);								// close config file
	
	/* Meta file */
	FILE * fmeta = fopenval(conf.meta_fname, fin_meta);
	
	logmeta(fmeta, conf);
	
	fclose(fmeta);
	
	/* Close Log file if given in config */
	if (logfile != NULL) { 	// close logfile if opened
		fclose(logfile); 	// log file path is optional parameter in config
	}						// it may be absent in config if log mode is monitor
	
    return 1;
}


/* Other Functions */
bool streq(const char *str1, const char *str2){
	// returns true if str1 == str2, false otherwise
	return strcmp(str1, str2) == 0;
}


int log(const char *format, ...){
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
	// log error message and exit
	va_list arg;
	va_start(arg, format);
	if (log2mon) { vfprintf(stdout, format, arg); }
	if (log2file) { vfprintf(logfile, format, arg); }
	va_end(arg);
	exit(0);
}


void strtrim(char* str) {
	// remove leading/trailing/multiple whitespace from str
	if (str == NULL) { return; }
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
  		str[i] = tolower(str[i]);
	return str;
}


void setlogmode(FILE *fconf){
	/* Sets logger to monitor/file/both using global flags log2mon, log2file
	* based on config file to be used by functions log() and logerror() */
	
	bool validlogmode = 0, validlogfile = 0;
	char line[256];
	while (fgets(line, sizeof(line), fconf)){
		strtrim(line);						// Remove leading/trailing/multiple whitespace
		char* key = strtok(line, ":");		// key = before :
		char* value = strtok(NULL, ":");	// value = after :
		strtrim(key); strtrim(value);		// Remove leading/trailing/multiple whitespace
		key = tolowerstr(key);				// Config file is case insensitive
		value = tolowerstr(value);
		if (key == NULL) {continue;}		// Empty line
		
		if (streq(key, "log")){
			if (value == NULL) break;
			
			char *mode = strrchr(value, ' ') + 1;
			if (streq(mode, "file"))		{ log2mon = 0; log2file = 1;}
			else if (streq(mode, "monitor")){ log2mon = 1; log2file = 0;}
			else if (streq(mode, "both"))	{ log2mon = 1; log2file = 1;}
			else {logerror("Error -- Invalid logging mode: %s", value);}
			validlogmode = 1;
		} 
		else if( streq(key, "log file path") && value != NULL ){
			strcpy(log_fname, value);
			logfile = fopen(log_fname, "w");
			validlogfile = 1;
		}
	}
	
	/* Exit program without logging anything if log mode not specified */
	if (!validlogmode) 
		logerror("Error -- No logging mode specified in config file"); 
	
	/* Log file path is required only if log to file or log to both */
	if (log2file && (!validlogfile)){
		printf("Error -- Invalid or no logfile specified");
		exit(1);
	}
	fseek(fconf, 0, SEEK_SET);	// Reset config file pointer for logconfig(FILE *)
}


FILE *fopenval(const char* fname, Fin fin){
	// Open & validate config or meta file
	
	char* ext = strchr(fname, '.');
	if ( !streq(ext, fin.ext) ){			/* Validate Extension */
		logerror("Error -- Invalid %s file extension: %s", fin.type, ext);
	}
	
	FILE* f = fopen(fname, "r");			/* Open file */
	
	if (f == NULL){							/* Validate file exists */
		logerror("Error -- cannot open %s file, invalid filename: %s", fin.type, fname);
	}
	
	fseek (f, 0, SEEK_END);
	long size = ftell(f);
	if (0 == size) {						/* Validate not empty file */
		logerror("Error -- %s file is empty\n", fin.type);
	}
	fseek(f, 0, SEEK_SET);					// Reset file pointer to read again

	return f;			
}


int verify_n_log(char* val, const char *part){
	// checks if cycle ms exists & nonnegative, part = processor, memory etc.
	if (val == NULL){ logerror("\nError -- '%s' cycle time is missing\n", part); }
	int v = atoi(val);
	if (v < 0) { logerror("\nError -- Invalid '%s' cycle time: %d\n", part, v); }
	log("%s = %d ms/cycle\n", part, v); 
	return v;
}


struct Config logconfig(FILE *fconf){
	// Logs config data to monitor/file/both
	
	bool metafilegiven = 0;
	struct Config conf;
	char line[256];							
	
	log("Configuration File Data\n");
	
	while (fgets(line, sizeof(line), fconf)){
		strtrim(line);						// Remove leading/trailing/multiple whitespace
		char* key = strtok(line, ":");		// key = before :
		char* value = strtok(NULL, ":");	// value = after :
		strtrim(key); strtrim(value);		// Remove leading/trailing/multiple whitespace
		key = tolowerstr(key);				// Config file is case insensitive
		value = tolowerstr(value);
		if (key == NULL) {continue;}		// Empty line
		
		if (streq(key, "monitor display time {msec}")){
			conf.monitor = verify_n_log(value, "Monitor");
		} else if(streq(key, "processor cycle time {msec}")){
			conf.processor = verify_n_log(value, "Processor"); 
		} else if(streq(key, "mouse cycle time {msec}")){
			conf.mouse = verify_n_log(value, "Mouse");
		} else if(streq(key, "hard drive cycle time {msec}")){
			conf.hdd = verify_n_log(value, "Hard Drive");
		} else if(streq(key, "keyboard cycle time {msec}")){
			conf.keyboard = verify_n_log(value, "Keyboard");
		} else if(streq(key, "memory cycle time {msec}")){
			conf.memory = verify_n_log(value, "Memory");
		} else if(streq(key, "printer cycle time {msec}")){
			conf.printer = verify_n_log(value, "Printer");
		} else if(streq(key, "version/phase")){
			strcpy(conf.version, value);
		} else if(streq(key, "file path")){
			if (value == NULL){ 
				logerror("Error -- no meta file specified!\n"); 
			} else { 
				strcpy(conf.meta_fname, value);
				metafilegiven = 1;
			}
		} else if(streq(key, "log")){
			if (log2mon && !log2file){
				sprintf(conf.logmode, "%s", "monitor");
			} else if (!log2mon && log2file){
				sprintf(conf.logmode, "%s", log_fname);
			} else if (log2mon && log2file){ 
				sprintf(conf.logmode, "monitor and %s", log_fname);
			}
			log("Logged to: %s\n\n", conf.logmode);
		} else if(streq(key, "log file path")){
			strcpy(conf.log_fname, log_fname);
		} else if(streq(key, "start simulator configuration file")){ ;
		} else if(streq(key, "end simulator configuration file")){ ;
		} else 
			logerror("Error -- Invalid config line: %s\n", line);
		}
	
	if (!metafilegiven){ logerror("Error -- no meta file specified!\n"); }
	return conf;
}


void logmeta(FILE *fmeta, struct Config conf){
	/* Read meta file into buffer string */
	char *buffer = 0;			// stores meta-data file as string
	long length;
	
	fseek (fmeta, 0, SEEK_END);
	length = ftell(fmeta);
	fseek (fmeta, 0, SEEK_SET);
	buffer = (char *) malloc(length);
	if (!buffer) logerror("Error -- reading meta-data file");
	fread (buffer, 1, length, fmeta);
	
	
	/* Detect Start Program Meta-Data Code: */
	char *token, *copy, *ptr = strdup(buffer);
	bool startmeta = 0;			// true if start meta-data present
	
	while ((token = strsep(&ptr, ":;.")) != NULL) {
		strtrim(token);
		copy = strdup(token);
		copy = tolowerstr(copy);
		
		if (streq(copy, "start program meta-data code")){
			startmeta = 1;
			break;
		}
    }
	/* Raise error if Start Program Meta-Data Code: absent */
	if (!startmeta) 
		logerror("Error -- Start Program Meta-Data Code: not found\n");
	
	
	/* Validate and log each meta data */
	log("Meta-Data Metrics\n");
	
	char *code, *desc, *cycle;	// meta-data code, descriptor, cycles
	bool endmeta = 0;			// true if program terminates normally 
	
	while ((token = strsep(&ptr, ":;.")) != NULL) {
		strtrim(token);
		copy = strdup(token);
		copy = tolowerstr(copy);

		if (streq(copy, "end program meta-data code")){	// End of meta file
			endmeta = 1;
			break;
		}
		
		char *ptr1 = strdup(token);
		code = strsep(&ptr1, "{(");		// meta code
		desc = strsep(&ptr1, "})");		// meta descriptor
		cycle = strsep(&ptr1, ")}");	// meta cycles
			
		strtrim(code); strtrim(desc); strtrim(cycle);	// remove whitespace
		desc = tolowerstr(desc);						// descriptor is case insensitive
		
		/* Empty meta-data line */
		if (code == NULL || *code == '\0') continue;
		if (streq(code, " ") && (desc==NULL) && (cycle==NULL))
			continue;
		
		/* start == begin, end == finish */
		if (streq(desc, "start")) desc = strdup("begin");
		if (streq(desc, "end")) desc = strdup("finish");
		
		//log("%s %s %s %s\n", token, code, desc, cycle);		// Debug
								
		
		/* Validate Code and descriptor */
		char SA[][20] {"begin", "finish"};
		char P[][20] {"run"};
		char I[][20] {"hard drive", "keyboard", "mouse"};
		char O[][20] {"hard drive", "printer", "monitor"};
		char M[][20] {"block", "allocate"};
		
		char (*d)[20];
		if (*code == 'S' || *code == 'A') d = SA;
		else if (*code == 'P') d = P;
		else if (*code == 'I') d = I;
		else if (*code == 'O') d = O;
		else if (*code == 'M') d = M;
		else
			logerror("\nInvalid meta-data code %s in meta-data %s", code, token);
		
		bool validdesc = 0;
		for(int i=0; i<10; i++)
			if (streq(desc, d[i]))
				validdesc = 1;
		
		if (!validdesc)
			logerror("\nError -- Invalid descriptor '%s', for code %s, in meta-data '%s'\n", desc, code, token);

		
		/* Validate cycles */
		if ((cycle==NULL) || (streq(cycle, " ")) || (*cycle=='\0'))
			logerror("\nError -- missing cycle value in meta-data '%s'", token);
		
		for(int i=0; i < strlen(cycle); i++)
			if (!isdigit(cycle[i]))
				logerror("\nError -- invalid cycles '%s' in meta-data '%s'", cycle, token);
		
		int c = atoi(cycle);
		if (c < 0)
			logerror("\nError -- negative cycles '%s' in meta-data '%s'", cycle, token);
		
		/* Calculate total time */
		int mspc;			// millisescond per cycle
		
		if (streq(desc, "hard drive"))	mspc = conf.hdd;
		else if (streq(desc, "keyboard"))	mspc = conf.keyboard;
		else if (streq(desc, "mouse"))	mspc = conf.mouse;
		else if (streq(desc, "monitor"))	mspc = conf.monitor;
		else if (streq(desc, "run"))	mspc = conf.processor;
		else if (streq(desc, "allocate"))	mspc = conf.memory;
		else if (streq(desc, "printer"))	mspc = conf.printer;
		else if (streq(desc, "block"))	mspc = conf.memory;
		else mspc = 0;
		
		int mstot = c*mspc;	// Total time for operation
		
		if (mstot == 0)		// begin, finish can have 0 cycles, others cannot
			if ( !streq(desc,"begin") && !streq(desc,"finish") )
				logerror("\nError -- '%s' cannot operate for 0 cycles: %s", desc, token);
			
		if (c > 0){
			if ( streq(desc,"begin") || streq(desc,"finish") )
				logerror("\nError -- '%s' cannot operate for '%d' cycles: %s", desc, c, token);
			else
				log("%s - %d ms\n", token, mstot);
		}
    }
	
	if (!endmeta) 
		logerror("\nError -- End Program Meta-Data Code: not found\n");

}