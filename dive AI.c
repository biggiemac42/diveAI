/* DIVE AI */
/* Brett Berger 19/12/2017
 * important contributions from "Grimy" and "12345ieee" via discord
 * Credit for this game to Alex Fink and the credits he gives
 * at https://alexfink.github.io/dive/
 */

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 16 uint32_ts for the board
 *
 * 0  1  2  3
 * 4  5  6  7
 * 8  9 10 11
 *12 13 14 15
 */

/* allow up to 29 seeds but only use the first numSeeds elements of the list in practice */
/* 26 is so that sizeof a lookahead tree struct (later) is 192 bytes, which is a nice number */

typedef struct ds {
	uint32_t board[16];
	uint32_t seeds[26];
	uint32_t numSeeds;
	uint32_t score;

	// Important info for evaluation function is carried by the diveState

	uint8_t biggestSeed;
	uint8_t secondBiggestSeed;
	uint8_t emptyTiles;
	
	bool gameOver;
} diveState;

typedef enum {
	Up,
	Right,
	Down,
	Left
} dirType;

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
	}
}

void updateSeeds(diveState *myState)
{
	/* array of flags: bit n determines if the nth seed is still on the board */
	uint32_t active = 0;
	uint32_t vals[16] = {0};

	for (uint32_t i = 0; i < 16; ++i)
	{
		vals[i] = myState->board[i];
		for (uint32_t j = 0; j < myState->numSeeds && vals[i] > 1; ++j)
			while (vals[i] % myState->seeds[j] == 0)
			{
				vals[i] /= myState->seeds[j];
				active |= 1 << j;
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
		for (uint32_t j = 0; j < 4; ++j)
		{
			uint32_t val = myState->board[getIndex(i + j, dir)];
			if (!val)
				continue;
			uint32_t newVal = newBoard[getIndex(top, dir)];
			if (!newVal)
			{
				newBoard[getIndex(top, dir)] = val;
				myState->emptyTiles -= 1;
			}
			else {
				uint32_t max = newVal > val ? newVal : val;
				uint32_t min = newVal < val ? newVal : val;
				if (max % min)
				{
					newBoard[getIndex(++top, dir)] = val;
					myState->emptyTiles -= 1;
				}
				else
				{
					newBoard[getIndex(top++, dir)] += val;
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

diveState newSpawn(diveState myState, uint32_t *rng)
{
	uint32_t numOptions;
	diveState *options = spawnOptions(myState, &numOptions);
	*rng = rand() % numOptions;
	diveState ret = options[*rng];
	free(options);
	return ret;
}

void printBoard(diveState myState)
{
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
}

/* The AI is one player in an unbalanced two-player game
 *
 * the game can be fully described by a pattern of values
 * describing choices.  The adversary gets 2 moves, then you
 * get 1 and it gets 1, alternating until no moves can be made.
 *
 * The adversary gets a variable number of options that can be
 * determined by the diveState.  The AI gets 4 options.  thus a 
 * game might look like {14, 12, 3, 10, 1, 7, 1, 15, etc...}.
 * There is thus an injective map from games of DIVE to lists
 * of uint32_tegers, though I cannot determine constraints such that
 * it is a bijection.
 *
 * The adversary (by default) selects a random uniform choice among
 * its options, equivalently a random branch of the lookahead tree.
 * For AI's sake, it's valid just to have the adversary's input be
 * a single call to "rand()", since the AI is holding all the
 * possibilities in an array already and thus knows how to interpret it.
 */

/* We define a lookahead tree struct.
 *
 * The tree will have a node for every state following the AI move only, and
 * evaluate only these states.  Nodes will hold:
 *    the state under consideration
 *    a pointer to the array of deeper nodes, NULL if a leaf
 *    the count of leaves in the array, zero if not yet malloc-ed
 *
 * since the branching follows the AI move and there are 4 options, there
 * will always be a multiple of 4 leaves.
 */

typedef struct lt {
	diveState myState;
	struct lt *leaves;
	uint32_t numLeaves;
} lookaheadTree;

/* We will be doing heap allocations, so all eliminated branches will
 * need to be freed from the leaves up.  Define a recursive free:
 */

void freeNode(lookaheadTree node)
{
	for (uint32_t i = 0; i < node.numLeaves; ++i)
		freeNode(node.leaves[i]);
	free(node.leaves);
}

/* promote a leaf to a node by computing its children.  If called
 * on a node with children, have no effect.
 */

void addChildren(lookaheadTree *parent)
{
	if (parent->numLeaves)
		return;

	uint32_t numOptions;
	diveState *options = spawnOptions(parent->myState, &numOptions);
	parent->numLeaves = 4*numOptions;
	parent->leaves = malloc(parent->numLeaves * sizeof(lookaheadTree));

	for (uint32_t i = 0; i < numOptions; ++i)
	{
		diveState orig = options[i];
		diveState tmp = orig;
		shift(&tmp, Up);
		parent->leaves[4*i] = (lookaheadTree) {tmp, NULL, 0};
		tmp = orig;
		shift(&tmp, Right);
		parent->leaves[4*i+1] = (lookaheadTree) {tmp, NULL, 0};
		tmp = orig;
		shift(&tmp, Down);
		parent->leaves[4*i+2] = (lookaheadTree) {tmp, NULL, 0};
		tmp = orig;
		shift(&tmp, Left);
		parent->leaves[4*i+3] = (lookaheadTree) {tmp, NULL, 0};
	}

	free(options);
}

/* A function to expand the tree to a given depth past a given root
 */

void computeToDepth(lookaheadTree *root, uint32_t depth)
{
	addChildren(root); // No effect if already a parent
	if (depth > 1)
		for (uint32_t i = 0; i < root->numLeaves; ++i)
			computeToDepth(root->leaves + i, depth - 1);
}


/* Ultimately a decision must be made.  The top level AI will hold 4
 * lookaheadTrees, one for each direction.  Each tree must be evaluated
 * and a winner chosen.  The way I will do this is recursively.  Leaves
 * compute the fitness of the contained state, while parents compute the
 * expected fitnesses of their leaves for each of the 4 movement options
 * and return the best - the AI plays to maximize expected board fitness.
 */

/* A good fitness function is important to the success of the AI
 * Currently, the important things to maximize are the score, the maximum seed,
 * the difference between maximum seed and the next highest, and the number of
 * empty tiles.
 *
 * To minimize - the number of seeds (both for ease of game and lookahead!)
 *
 * Parameters here have been tuned very little.  Other heuristics are probably
 * important but hard to put into code.  Having large seeds for which
 * 2*s+1 is prime or at least not divisible by other things on the board is good.
 * My personal strategy will often go out of the way to unlock 89, for example.
 * Position of the large values is important but lookahead should be able to convert
 * that into an expected score increase.
 *
 * A few heuristics are directly reflected in this fitness function if allowed
 * to look sufficiently deep.  To that end, the ability to look to depth 3 or 4
 * would improve performance greatly.
 */

float evaluate(diveState *myState)
{
	return (myState->gameOver) ? 0.0 : myState->score +
	                                   3.0 * myState->biggestSeed +
	                                   2.0 * (myState->biggestSeed - myState->secondBiggestSeed) +
	                                   40.0 * myState->emptyTiles +
	                                   3100.0 / myState->numSeeds;
}

float evaluateTree(lookaheadTree *node)
{
	if (!node->numLeaves)
		return evaluate(&(node->myState));

	float upScore = 0;
	float rightScore = 0;
	float downScore = 0;
	float leftScore = 0;

	for (uint32_t i = 0; i < node->numLeaves;)
	{
		upScore += evaluateTree(node->leaves + i++);
		rightScore += evaluateTree(node->leaves + i++);
		downScore += evaluateTree(node->leaves + i++);
		leftScore += evaluateTree(node->leaves + i++);
	}

	float udMax = (upScore > downScore)  ? upScore : downScore;
	float lrMax = (leftScore > rightScore)  ? leftScore : rightScore;
	float hvMax = (udMax > lrMax) ? udMax : lrMax;

	return hvMax / node->numLeaves * 4;
}


/* To have an intlist record of the game, I just allocate a static array
 * to hold the moves.  10000 ints shouldn't be memory that is missed.
 */
static uint32_t MAX_NUM_MOVES = 10000;

/* AI evaluates states after DEPTH + 1 moves and DEPTH spawns
 * Current implementation completes 1000 games in ~5 seconds at
 * depth 1, 1000 games in ~an hour at depth 2.  Haven't completed
 * a game at depth 3 yet.
 */
static uint32_t DEPTH = 2;

/* Returns the intlist directly, and score and number of moves
 * indirectly.
 */
uint32_t *playGame(uint32_t *score, uint32_t *nthMove)
{
	uint32_t *summary;
	diveState game;
	diveState *options;
	lookaheadTree myTree[4];
	lookaheadTree temp;
	float fitness[4];
	float bestFitness;
	dirType myMove;
	uint32_t numOptions;

	game = (diveState) {{0}, {2}, 1, 0, false};
	*nthMove = 0;
	summary = malloc(MAX_NUM_MOVES * sizeof *summary);
	game = newSpawn(game, summary + (*nthMove)++);
	game = newSpawn(game, summary + (*nthMove)++);

	diveState tmp = game;
	shift(&tmp, Up);
	myTree[Up] = (lookaheadTree) {tmp, NULL, 0};
	tmp = game;
	shift(&tmp, Right);
	myTree[Right] = (lookaheadTree) {tmp, NULL, 0};
	tmp = game;
	shift(&tmp, Down);
	myTree[Down] = (lookaheadTree) {tmp, NULL, 0};
	tmp = game;
	shift(&tmp, Left);
	myTree[Left] = (lookaheadTree) {tmp, NULL, 0};

	while(!game.gameOver)
	{
		bestFitness = -1.0;
		for (uint32_t i = 0; i < 4; ++i)
		{
			computeToDepth(&myTree[i], DEPTH);
			fitness[i] = evaluateTree(myTree + i);
			if (fitness[i] > bestFitness)
			{
				bestFitness = fitness[i];
				summary[*nthMove] = i;
			}
		}

		myMove = (dirType) summary[*nthMove];
		temp = myTree[myMove];

		// Prune unused branches - no memory leaks pls
		for (uint32_t i = 0; i < 4; ++i)
			if (i != myMove)
				freeNode(myTree[i]);
			options = spawnOptions(temp.myState, &numOptions);
		++(*nthMove);
		summary[*nthMove] = rand() % numOptions;
		game = options[summary[*nthMove]];
		free(options);
		//printBoard(game);

		for (uint32_t i = 0; i < numOptions; ++i)
			if (i != summary[*nthMove])
			{
				freeNode(temp.leaves[4*i]);
				freeNode(temp.leaves[4*i+1]);
				freeNode(temp.leaves[4*i+2]);
				freeNode(temp.leaves[4*i+3]);
			}

		myTree[0] = temp.leaves[4*summary[*nthMove]];
		myTree[1] = temp.leaves[4*summary[*nthMove]+1];
		myTree[2] = temp.leaves[4*summary[*nthMove]+2];
		myTree[3] = temp.leaves[4*summary[*nthMove]+3];
		++(*nthMove);
		free(temp.leaves);
	}

	(*score) = game.score;
	return summary;
}




static uint32_t NGAMES = 10;

uint32_t main()
{
	srand(time(NULL));
	uint32_t aiScore = 0;
	uint32_t aiHighScore = 0;
	uint32_t n10k = 0;
	uint32_t *summary;
	uint32_t score;
	uint32_t nthMove;
	
	for (int g = 0; g < NGAMES; ++g)
	{
		summary = playGame(&score, &nthMove);
		//for (uint32_t i = 0; i < nthMove; ++i)
		//	printf("%d\n", (*summary)[i]);

		printf("%d\n", score);
		aiScore += score;
		aiHighScore = (aiHighScore > score) ? aiHighScore : score;
		if (score > 10000)
			++n10k;
		free(summary);
	}

	printf("Mean : %d\n",aiScore / NGAMES);
	printf("Highest: %d\n",aiHighScore);
	printf("10K games: %d / %d", n10k, NGAMES);

	return 0;
}
