CC=gcc
CFLAGS=-Wall

sourcesClient= client2.c
objetsClient= $(sourcesClient:.c=.o)

client: $(objetsClient)
	$(CC) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o client