CFLAGS=-W -Wall -Werror -pedantic -std=c11 -Wimplicit-fallthrough
OBJS=jiffy.o

.PHONY=all clean

all: $(OBJS)

%.o: %.c jiffy.h
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) $(OBJS)
