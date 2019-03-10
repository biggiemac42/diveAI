#include "dive.h"
#include <stdio.h>
#include <string.h>

void printBoard(diveState myState)
{
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\nScore: %d\n\n", myState.score);
	printf("Seeds: ");
	for (uint32_t i = 0; i < myState.numSeeds; ++i)
		printf("%d ", myState.seeds[i]);


	printf("\n");
	for (uint32_t i = 0; i < 16; ++i)
	{
		myState.board[i] ? printf("%7d ", myState.board[i]) : printf("      . ");
		if (i % 4 == 3)
			printf("\n");
	}
	printf("\n");
	//printf("Max tiles: %d, %d\nMax seeds: %d, %d\n", myState.maxTile, myState.submaxTile, myState.biggestSeed, myState.secondBiggestSeed);
}

static uint32_t board90[16] = {12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3};

uint32_t getIndex(uint32_t index, dirType dir)
{
	switch (dir) {
		case Left:
		return index;
		case Down:
		return board90[index];
		case Right:
		return ~index & 15;
		case Up:
		return ~board90[index] & 15;
		default:
		return 0; // Never happens
	}
}

void updateSeeds(diveState *myState)
{
	/* array of flags: bit n determines if the nth seed is still on the board */
	uint32_t active = 0;
	uint32_t vals[16] = {0};

	myState->submaxTile = 0;
	myState->maxTile = 0;

	for (uint32_t i = 0; i < 16; ++i)
	{

		vals[i] = myState->board[i];

		if (!vals[i])
			continue;

		if (vals[i] > myState->maxTile)
		{
			myState->submaxTile = myState->maxTile;
			myState->maxTile = vals[i];
		}
		else if (vals[i] > myState->submaxTile)
			myState->submaxTile = vals[i];

		for (uint32_t j = 0, flag = 1; j < myState->numSeeds; ++j, flag <<=1)
		{
			if (vals[i] < myState->seeds[j])
				continue;
		    uint32_t r;
            uint32_t q;
            r = vals[i] % myState->seeds[j];
            q = vals[i] / myState->seeds[j];
			while (r == 0)
			{
                vals[i] = q;
                active |= flag;
                r = vals[i] % myState->seeds[j];
                q = vals[i] / myState->seeds[j];
			}
		}
	}

	uint32_t seed;
	uint32_t newNumSeeds = 0;
	myState->biggestSeed = 0;
	myState->secondBiggestSeed = 0;

	for (uint32_t i = 0; i < myState->numSeeds; ++i)
		if (active & 1 << i)
		{
			seed = myState->seeds[i];
			myState->seeds[newNumSeeds++] = seed;
			if (seed > myState->biggestSeed)
			{
				myState->secondBiggestSeed = myState->biggestSeed;
				myState->biggestSeed = seed;
			}
			else if (seed > myState->secondBiggestSeed)
				myState->secondBiggestSeed = seed;
		}
		else
			myState->score += myState->seeds[i]; // Eliminated is worth points apparently

	/* bookkeepping to make sure we don't unlock the same seed twice in a turn */
	uint32_t remaining = newNumSeeds;
	bool redundant;

	for (uint32_t i = 0; i < 16; ++i)
	{
		if (vals[i] <= 1)
			continue;
		redundant = false;
		for (uint32_t j = remaining; j < newNumSeeds; ++j)
		{
			redundant = (vals[i] == myState->seeds[j]);
			if (redundant)
				break;
		}
		if (!redundant)
		{
			seed = vals[i];
			myState->seeds[newNumSeeds++] = seed;
			if (seed > myState->biggestSeed)
			{
				myState->secondBiggestSeed = myState->biggestSeed;
				myState->biggestSeed = seed;
			}
			else if (seed > myState->secondBiggestSeed)
				myState->secondBiggestSeed = seed;
		}
	}

	myState->numSeeds = newNumSeeds;
}

void shift(diveState *myState, dirType dir)
{
	if (myState->gameOver)
		return;
	
	/* initialize to zero */
	uint32_t newBoard[16] = {0};
	myState->emptyTiles = 16;

	/* don't update seeds if the move was pure translation */
	bool dirty = false;

	for (uint32_t i = 0; i < 16; i += 4) {
		uint32_t top = i;
		uint32_t topDir = getIndex(top, dir);
		for (uint32_t j = 0; j < 4; ++j)
		{
			uint32_t val = myState->board[getIndex(i + j, dir)];
			if (!val)
				continue;
			uint32_t newVal = newBoard[topDir];
			if (!newVal)
			{
				newBoard[topDir] = val;
				myState->emptyTiles -= 1;
			}
			else {
				uint32_t max = newVal > val ? newVal : val;
				uint32_t min = newVal < val ? newVal : val;
				if (max % min)
				{
					topDir = getIndex(++top, dir);
					newBoard[topDir] = val;
					myState->emptyTiles -= 1;
				}
				else
				{
					newBoard[topDir] += val;
					topDir = getIndex(++top, dir);
					myState->score += min;
					dirty = true;
				}
			}
		}
	}

	myState->gameOver = !memcmp(myState->board, newBoard, 64);
	memcpy(myState->board, newBoard, 64);

	if (dirty)
		updateSeeds(myState);
}

/* Explicitly returns the list of options, returns by pointer the number of them */
/* Return value of this function must be freed */
diveState *spawnOptions(diveState myState, uint32_t *numOptions)
{
	if (myState.gameOver)
	{
		*numOptions = 1;
		diveState *dest = malloc(sizeof *dest);
		*dest = myState;
		return dest;
	}

	uint32_t spaces = 0;
	uint32_t locs[15];

	for (uint32_t i = 0; i < 16; ++i)
		if (!myState.board[i])
			locs[spaces++] = i;

	*numOptions = spaces * myState.numSeeds;

	diveState *dest = malloc(*numOptions * sizeof *dest);

	for (uint32_t i = 0; i < spaces; ++i)
		for (uint32_t j = 0; j < myState.numSeeds; ++j)
		{
			dest[j*spaces + i] = myState;
			dest[j*spaces + i].board[locs[i]] = myState.seeds[j];
			dest[j*spaces + i].emptyTiles -= 1;
		}

	return dest;
}

void newSpawn(diveState *myState, uint32_t *rng)
{
	uint32_t numOptions;
	diveState *options = spawnOptions(*myState, &numOptions);
	*rng = rand() % numOptions;
	*myState = options[*rng];
	free(options);
}
