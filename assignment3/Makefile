CFLAGS = -Wall -g   

all: diskinfo diskget disklist diskput

diskinfo: diskinfo.o
	gcc $(CFLAGS) diskinfo.c -o diskinfo

diskget: diskget.o
	gcc $(CFLAGS) diskget.c -o diskget
	
disklist: disklist.o
	gcc $(CFLAGS) disklist.c -o disklist
	
diskput: diskput.o
	gcc $(CFLAGS) diskput.c -o diskput

clean: 
	rm -rf *.o *.exe
