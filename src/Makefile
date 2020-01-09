CFLAGS=-O2 -W -Wall -Wextra -Werror -Wimplicit-fallthrough -pedantic -std=c11
# CFLAGS= -W -Wall -Werror -pedantic -std=c11 -Wimplicit-fallthrough -g -pg
OBJS=jiffy.o jiffy-test.o tests/test-parser.o tests/test-tree.o
APP=jiffy-test

.PHONY=all clean test

all: $(APP)

$(APP): $(OBJS)
	$(CC) -o $(APP) $(OBJS)

%.o: %.c jiffy.h
	$(CC) -c -o $@ $(CFLAGS) $<

test: $(APP)
	./$(APP) parser ./tests/data/corpus.txt
	./$(APP) tree ./tests/data/corpus.txt

clean:
	$(RM) $(OBJS) $(APP)
