CC=gcc
CFLAGS=-Wall

sourcesServeur= server2.c
objetsServeur= $(sourcesServeur:.c=.o)

server: $(objetsServeur)
	$(CC) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o server