all: clean sim4

sim4: sim4.c 
	gcc -pthread -lpthread -lrt -o sim4 sim4.c
	
clean:
	rm -f sim4 *.exe *.stackdump *.lgf
