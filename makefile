CC=gcc
CFLAGS = -Wall -O3 -g -std=c99

diveAI: diveAI.c
    $(CC) $(CFLAGS) diveAI.c -o diveAI