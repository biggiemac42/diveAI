#include "AI.h"

#include <stdio.h> // debugging
#include <math.h> // to have other options in eval

/* The tuning knobs for the eval function are defined here as constants */

/* Above this score the depth takes on a minumum of 1. */
static uint32_t DEPTH_1_SCORE = 6500;

/* Above this score the depth takes on a minimum of 2. */
static uint32_t DEPTH_2_SCORE = 250000;


/* The board's score when evaluated is the weighted sum of 5 quantities:
 * number of empty tiles
 * inverse seed count
 * a function of biggest seed
 * a function of second biggest seed
 * a function of score
 *
 * The intention is to encourage games with exactly one big seed, so the weight
 * on the second biggest seed is negative.
 */
static float EMPTY_TILE_WEIGHT = 70.0;
static float INV_SEED_COUNT_WEIGHT = 3100.0;
static float BIGGEST_SEED_WEIGHT = 1.5;
static float SECOND_SEED_WEIGHT = -1.0;
static float SCORE_WEIGHT = 8.0;

/* The function for seeds and score behavies linearly for small values and logarithmic
 * asymptotically.  We precompute all values below 100k, and then call this function
 * for higher values */

float linloghelper(uint32_t value)
{
	float x = (float) value;
	return 1000*(3*x/(x+1000) + log(x+1000) - log(1000));
}

static float linlogHelpList[100000];
static bool listPopulated = false;

void populateHelpList()
{
	for (int i = 0; i < 100000; ++i)
		linlogHelpList[i] = linloghelper(i);
	listPopulated = true;
}

float linlog(uint32_t value)
{
	return (listPopulated && value < 100000) ? linlogHelpList[value] : linloghelper(value);
}



/* We will be doing heap allocations, so all eliminated branches will
 * need to be freed from the leaves up.  Define a recursive free:
 */
void freeNode(lookaheadTree *node)
{
	for (uint32_t i = 0; i < node->numLeaves; ++i)
		freeNode(node->leaves + i);
	free(node->leaves);
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


uint32_t gcd(uint32_t a, uint32_t b)
{
	uint32_t r;
	while (b!=0)
	{
		r = a % b;
		a = b;
		b = r;
	}
	return a;
}

float evaluate(diveState *myState)
{
	if (myState->gameOver)
		return 0.0;
	else
		return SCORE_WEIGHT * linlog(myState->score)
			 + BIGGEST_SEED_WEIGHT * linlog(myState->biggestSeed)
			 + SECOND_SEED_WEIGHT * linlog(myState->secondBiggestSeed)
	         + EMPTY_TILE_WEIGHT * myState->emptyTiles
	         + INV_SEED_COUNT_WEIGHT / myState->numSeeds;
}

float evaluateTree(lookaheadTree *node)
{
	if (node->numLeaves == 0)
	{
		diveState tmp = node->myState;
		shift(&tmp, Up);
		float maxScore = evaluate(&tmp);
		tmp = node->myState;
		shift(&tmp, Right);
		float tmpScore = evaluate(&tmp);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		tmp = node->myState;
		shift(&tmp, Down);
		tmpScore = evaluate(&tmp);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		tmp = node->myState;
		shift(&tmp, Left);
		tmpScore = evaluate(&tmp);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		return maxScore;
	}

	float upScore = 0;
	float rightScore = 0;
	float downScore = 0;
	float leftScore = 0;

	for (uint32_t i = 0; i < node->numLeaves; i += 4)
	{
		upScore += evaluateTree(node->leaves + i);
		rightScore += evaluateTree(node->leaves + i + 1);
		downScore += evaluateTree(node->leaves + i + 2);
		leftScore += evaluateTree(node->leaves + i + 3);
	}

	float udMax = (upScore > downScore)  ? upScore : downScore;
	float lrMax = (leftScore > rightScore)  ? leftScore : rightScore;
	float hvMax = (udMax > lrMax) ? udMax : lrMax;

	return hvMax / node->numLeaves;
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

/* Returns the intlist directly, and score and number of moves
 * indirectly.
 */
uint32_t *playGame(uint32_t *score, uint32_t *nthMove, uint32_t depth, bool verbose, uint32_t *resetTicker)
{
	uint32_t *summary;
	diveState *options;
	diveState game;
	lookaheadTree myTree[4];
	lookaheadTree temp;
	float fitness[4];
	float bestFitness;
	dirType myMove;
	uint32_t numOptions;
	uint32_t myDepth;


	reset: 
	game = (diveState) {{0}, {2}, 1, 2, 2, 2, 0, 16, false};
	*nthMove = 0;
	summary = malloc(MAX_NUM_MOVES * sizeof *summary);
	newSpawn(&game, summary + (*nthMove)++);
	newSpawn(&game, summary + (*nthMove)++);
	updateSeeds(&game);

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
		if (game.score < 25000 && game.emptyTiles < 4)
		{
			++(*resetTicker);
			free(summary);
			goto reset;
		}
		if (game.score < DEPTH_1_SCORE)
			myDepth = depth;
		else if (game.score < DEPTH_2_SCORE)
			myDepth = (depth > 1) ? depth : 1;
		else
			myDepth = (depth > 2) ? depth : 2;

		bestFitness = -1.0;
		for (uint32_t i = 0; i < 4; ++i)
		{			
			if (myDepth > 0)
				computeToDepth(myTree + i, myDepth);

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
				freeNode(myTree + i);

		
		options = spawnOptions(temp.myState, &numOptions);
		++(*nthMove);
		summary[*nthMove] = rand() % numOptions;
		game = options[summary[*nthMove]];
		free(options);

		if (verbose)
			printBoard(game);

		if (myDepth > 0)
		{
			for (uint32_t i = 0; i < numOptions; ++i)
				if (i != summary[*nthMove])
				{
					freeNode(temp.leaves + 4*i);
					freeNode(temp.leaves + 4*i + 1);
					freeNode(temp.leaves + 4*i + 2);
					freeNode(temp.leaves + 4*i + 3);
				}

			myTree[Up] = temp.leaves[4*summary[*nthMove]];
			myTree[Right] = temp.leaves[4*summary[*nthMove]+1];
			myTree[Down] = temp.leaves[4*summary[*nthMove]+2];
			myTree[Left] = temp.leaves[4*summary[*nthMove]+3];
			free(temp.leaves);
		}
		else
		{
			tmp = game;
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
		}
		++(*nthMove);
	}
	if (verbose)
		printBoard(game);

	(*score) = game.score;
	return summary;
}
