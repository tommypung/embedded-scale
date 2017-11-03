CC=gcc
CFLAGS=-g -std=gnu99 -DAPI_URL=$(API_URL)
DEPS=main.h serial.h vetek.h fork.h status.h
OBJ=main.o serial.o vetek.o fork.o status.o
SRCS=$(wildcard src/*.c)
OBJ=$(SRCS:.c=.o)
LIBS=-lcurl -lao -lm -lpthread

_DEPS=$(patsubst %,src/%,$(DEPS))
#_OBJ=$(patsubst %,src/%,$(OBJ))

all: main

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

#%.o: %.c $(_DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -v -f src/*.o src/*~ *.o *~ main
depend:
	makedepend -- $(CFLAGS) -- $(SRCS) -Ysrc
# DO NOT DELETE

src/vetek.o: src/vetek.h
src/api.o: src/api.h
src/main.o: src/fork.h src/api.h src/vetek.h src/serial.h src/sound.h
src/main.o: src/status.h
src/status.o: src/status.h
src/serial.o: src/serial.h
src/sound.o: src/sound.h
src/fork.o: src/status.h src/fork.h
