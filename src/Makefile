CFLAGS=-O2 -W -Wall -Wextra -Werror -Wimplicit-fallthrough -pedantic -std=c11
# CFLAGS=-O2 -W -Wall -Wextra -Werror -Wimplicit-fallthrough -pedantic -std=c11 -g -pg
OBJS=jiffy.o tests/main.o tests/test-set.o tests/parser.o tests/tree.o tests/builder.o
APP=jiffy-test

.PHONY=all clean test

all: $(APP)

$(APP): $(OBJS)
	$(CC) -o $(APP) $(OBJS)

%.o: %.c jiffy.h
	$(CC) -c -o $@ $(CFLAGS) $<

test: $(APP)
	./$(APP) all ./tests/corpus.txt

clean:
	$(RM) $(OBJS) $(APP)
