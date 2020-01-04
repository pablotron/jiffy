CFLAGS=-W -Wall -Werror -pedantic -std=c11 -Wimplicit-fallthrough
OBJS=jiffy.o test.o
APP=jiffy-test

.PHONY=all clean test

all: $(APP)

$(APP): $(OBJS)
	$(CC) -o $(APP) $(OBJS)

%.o: %.c jiffy.h
	$(CC) -c $(CFLAGS) $<

test: $(APP)
	./$(APP) "{}" '5' '{"foo":true,"bar":false,"baz":[null]}'

clean:
	$(RM) $(OBJS) $(APP)
