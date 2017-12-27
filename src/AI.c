#include "AI.h"

#include <stdio.h> // debugging

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

/* Attempt to give a tremendous boost to 89 and company, but only if the seed is alone */
uint32_t rateMySeeds(uint32_t biggestSeed, uint32_t secondBiggestSeed)
{
	if (biggestSeed < 89)
		return biggestSeed;
	uint32_t ret =  (!((biggestSeed + 1) % 90) && secondBiggestSeed < 23) ?
	                 biggestSeed * 10 : biggestSeed;
	return ret;
}

float evaluate(diveState *myState, bool cleaning)
{
	if (myState->gameOver)
		return 0.0;
	else if (cleaning)
		return 70.0 * myState->emptyTiles + 3100 / myState->numSeeds;
	else
		return myState->score +
			   3.0 * rateMySeeds(myState->biggestSeed, myState->secondBiggestSeed) +
	 		   ((myState->maxTile % myState->biggestSeed) ? 0.0 : 300.0) +
	 		   ((myState->submaxTile) ? 10.0 * myState->maxTile / myState->submaxTile : 100) +
	 		   ((myState->secondBiggestSeed) ? 10.0 * myState->biggestSeed / myState->secondBiggestSeed : 100) +
	           70.0 * myState->emptyTiles +
	           3100.0 / myState->numSeeds;
}

float evaluateTree(lookaheadTree *node, bool cleaning)
{
	if (node->numLeaves == 0)
	{
		diveState tmp = node->myState;
		shift(&tmp, Up);
		float maxScore = evaluate(&tmp, cleaning);
		tmp = node->myState;
		shift(&tmp, Right);
		float tmpScore = evaluate(&tmp, cleaning);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		tmp = node->myState;
		shift(&tmp, Down);
		tmpScore = evaluate(&tmp, cleaning);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		tmp = node->myState;
		shift(&tmp, Left);
		tmpScore = evaluate(&tmp, cleaning);
		maxScore = (tmpScore > maxScore) ? tmpScore : maxScore;
		return maxScore;
	}

	float upScore = 0;
	float rightScore = 0;
	float downScore = 0;
	float leftScore = 0;

	for (uint32_t i = 0; i < node->numLeaves; i += 4)
	{
		upScore += evaluateTree(node->leaves + i, cleaning);
		rightScore += evaluateTree(node->leaves + i + 1, cleaning);
		downScore += evaluateTree(node->leaves + i + 2, cleaning);
		leftScore += evaluateTree(node->leaves + i + 3, cleaning);
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
uint32_t *playGame(uint32_t *score, uint32_t *nthMove, uint32_t depth, bool verbose)
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
	bool cleaning;

	//game = (diveState) {{0, 0, 0, 11, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 818}, {2, 11, 409}, 3, 818, 11, 409, 11, 11, false};
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
		bestFitness = -1.0;
		for (uint32_t i = 0; i < 4; ++i)
		{
			cleaning = myTree[i].myState.score < 777;
			
			if (depth > 0)
				computeToDepth(myTree + i, depth);

			fitness[i] = evaluateTree(myTree + i, cleaning);

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

		if (verbose)
			printBoard(game);

		if (depth > 0)
		{
			for (uint32_t i = 0; i < numOptions; ++i)
				if (i != summary[*nthMove])
				{
					freeNode(temp.leaves[4*i]);
					freeNode(temp.leaves[4*i+1]);
					freeNode(temp.leaves[4*i+2]);
					freeNode(temp.leaves[4*i+3]);
				}

			myTree[Up] = temp.leaves[4*summary[*nthMove]];
			myTree[Right] = temp.leaves[4*summary[*nthMove]+1];
			myTree[Down] = temp.leaves[4*summary[*nthMove]+2];
			myTree[Left] = temp.leaves[4*summary[*nthMove]+3];
			++(*nthMove);
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
	}
	if (verbose)
		printBoard(game);

	(*score) = game.score;
	return summary;
}
