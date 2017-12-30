CC=gcc
CFLAGS = -Wall -O3 -g -std=c99 -lm
DEPS = src/dive.h src/AI.h
AIOBJS = src/dive.o src/AI.o src/diveAI.o
REPLAYOBJS = src/dive.o src/replay.o

all: diveAI replay

src/AI.o: src/dive.h src/AI.h
src/dive.o: src/dive.h
src/diveAI.o: src/dive.h src/AI.h
src/replay.o: src/dive.o

diveAI: $(AIOBJS)
	$(CC) $(CFLAGS) -o diveAI $(AIOBJS)

replay: $(REPLAYOBJS)
	$(CC) $(CFLAGS) -o replay $(REPLAYOBJS)


clean:
	rm src/*.o