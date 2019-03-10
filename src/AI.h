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

#ifndef AI_H_INCLUDED
#define AI_H_INCLUDED

#include "dive.h"


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

void populateHelpList();

void freeNode(lookaheadTree *node);
void addChildren(lookaheadTree *parent);
void computeToDepth(lookaheadTree *root, uint32_t depth);
float evaluate(diveState *myState);
float evaluateTree(lookaheadTree *node);
uint32_t *playGame(uint32_t *score, uint32_t *nthMove, uint32_t depth, bool verbose, uint32_t *resetTicker, bool canReset);

#endif
