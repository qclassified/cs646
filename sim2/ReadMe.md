# Project 1 - Sim01

#### CS 646 Principles of Computer Operating Systems

#### University of Nevada, Reno

#### Farhan Sadique


## Make and run

```
make
./sim1 sim1.conf
```

## Program Flow
* Open and verify (details below) config file
* Read through config file and sets log mode to monitor/file/both
* Read each line of config file -> store config in struct -> log config
* Close config file
* Open meta file
* Store entire content of meta file to buffer string
* Look for Start Program Meta-Data Code:
* Begin parsing meta-file after Start Program Meta-Data Code: is detected
* Read and validate (explained below) each meta-data code
* Log each meta-data code to monitor/file/both
* Stop when End Program Meta-Data Code: is detected
* If End Program Meta-Data Code: is not detected stop when EOF reached and raise error
* Close meta file

## Error Handling and Test Cases



#### Error logging mode

In the beginning, by default, program will log all errors to monitor. Before logging anything program tries to set the log mode (monitor/file/both) according to config file.

After logging mode is configured from config file ("Log to: monitor/file/both"), subsequent errors will be logged as per configured mode. 

For example if there is an error opening config file it will be logged to monitor, however if there is an error opening meta-data file it will be logged to configured stream (monitor/file/both).



#### Error logging sequence

Error logging will be sequential: the program will log values till an error is encountered, log error message, exit.

Example Output: 
Configuration File Data
Monitor = 20 ms/cycle
Processor = 10 ms/cycle
Mouse = 25 ms/cycle
Hard Drive = 15 ms/cycle
Keyboard = 50 ms/cycle
Memory = 30 ms/cycle
Error -- 'Printer' cycle time is missing



#### Wrong file extension

config and meta files must have .conf and .mdf extensions respectively. Log file can have any extension.

Example output: Error -- Invalid meta file extension: .txt



#### Wrong filename or missing file

Example Output: "Error -- cannot open meta file, invalid filename: test_a.mdf"



#### Empty file

Example: Error -- meta file is empty


#### Case insensitive config file, meta descriptor

All lines in config file are considered case insensitive

Example: 'Log: Log to Monitor' and 'log: log to Monitor' are both valid.

Only meta-data 'descriptors' (not 'code') are considered case insensitive

Example: M{allocate}2 and M{Allocate}2 are both valid but m{allocate}2 is invalid


#### Leading/trailing/multiple whitespace allowed

In both config and meta file leading, trailing, multiple whitespace (including \t etc.) are allowed between words. 

Which means empty lines and empty meta-data are allowed. 

Example: ; ; is considered empty meta-data and skipped without throwing error.



#### No/Missing logging mode

Example: 'Error -- No logging mode specified in config file'



#### Invalid logging mode

Example: Error -- Invalid logging mode: log to woods



#### Log file path is optional in config file

If logging mode is 'Log: Log to Monitor', then 'Log File Path: xx.xx' is optional in config file. 

However if logging mode is 'Log: Log to File' or 'Log: Log to Both' then program throws error if 'Log File Path: ' is missing in config file.

Example: Error -- Invalid or no logfile specified



#### Invalid config line

Example: Error -- Invalid config line: example of bad config line



#### Missing or invalid cycle time (ms) in config file

Example: Error -- 'Keyboard' cycle time is missing
Example: Error -- Invalid 'Monitor' cycle time: -20



#### Meta file reading starts after detecting start program meta-data code

All lines before "Start Program Meta-Data Code:" will be skipped in meta file



#### Missing Start/End Program Meta-Data Code:

Example: Error -- Start Program Meta-Data Code: not found
Example: Error -- End Program Meta-Data Code: not found


#### Meta-data code must be in [S, A, P, I, O, M]

Example: Invalid meta-data code X in meta-data X{begin}0



#### Meta-data descriptor must be in ["begin", "finish", "hard drive", "keyboard", "mouse", "monitor", "run", "allocate", "printer", "block"]

Example: Error -- Invalid descriptor 'vehicle' in meta-data 'O{vehicle}4'



#### Meta-data code, descriptor must match

M (Memory) cannot have start etc.

Example: Error -- Invalid descriptor 'begin', for code M, in meta-data 'M{start}2'



#### Cycle value must be given and positive for meta-data

Example: Error -- missing cycle value in meta-data 'O{printer}'
Example: Error -- invalid cycles '-4' in meta-data 'O{printer}-4'



#### Forgives common typos in meta file

Following are interchangeable in meta file:
{ and (
} and )
: and ; and .
stand and begin
end and finish

Program does not throw error for these common typos because it can still carry on without confusion. 

Example: I{hard drive)8 - 160 ms



#### Empty meta-data and whitespace allowed in meta file

Example: Following is a valid meta file 

```
;; ;	;
;	

Start    Program 	Meta-Data Code:

S{begin}0;
  
 End Program Meta-Data Code.

```



#### Begin, Finish cannot operate for more than 0 ms. Others cannot operate for 0 cycles.

Example: Error -- 'run' cannot operate for 0 cycles: P{run}0
Example: Error -- 'begin' cannot operate for '1' cycles: S{begin}1





































