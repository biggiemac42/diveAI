#ifndef DIVE_H_INCLUDED
#define DIVE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* 16 uint32_ts for the board
 *
 * 0  1  2  3
 * 4  5  6  7
 * 8  9 10 11
 *12 13 14 15
 */

/* allow up to 21 seeds but only use the first numSeeds elements of the list in practice */
/* 21 is so that sizeof a lookahead tree struct (later) is 192 bytes, which is a nice number */
typedef struct ds {
	uint32_t board[16];
	uint32_t seeds[21];
	uint32_t numSeeds;
	uint32_t score;

	// Important info for evaluation function is carried by the diveState
	uint32_t maxTile;
	uint32_t submaxTile;
	uint32_t biggestSeed;
	uint32_t secondBiggestSeed;
	uint8_t emptyTiles;
	
	bool gameOver;
} diveState;

void printBoard(diveState myState);

typedef enum {
	Up,
	Right,
	Down,
	Left
} dirType;

uint32_t getIndex(uint32_t index, dirType dir);
void updateSeeds(diveState *myState);
void shift(diveState *myState, dirType dir);
diveState *spawnOptions(diveState myState, uint32_t *numOptions);
void newSpawn(diveState *myState, uint32_t *rng);

#endif
