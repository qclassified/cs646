all: clean sim5

sim5: sim5.c 
	gcc -pthread -lpthread -lrt -o sim5 sim5.c
	
clean:
	rm -f sim5 *.exe *.stackdump *.lgf
