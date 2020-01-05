CFLAGS=-W -Wall -Wextra -Werror -Wimplicit-fallthrough -pedantic -std=c11
# CFLAGS= -W -Wall -Werror -pedantic -std=c11 -Wimplicit-fallthrough -g -pg
OBJS=jiffy.o test.o
APP=jiffy-test

.PHONY=all clean test

all: $(APP)

$(APP): $(OBJS)
	$(CC) -o $(APP) $(OBJS)

%.o: %.c jiffy.h
	$(CC) -c $(CFLAGS) $<

test: $(APP)
	./$(APP) < ./tests.txt

clean:
	$(RM) $(OBJS) $(APP)
