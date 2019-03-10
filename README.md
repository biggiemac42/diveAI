# diveAI

A C reimplementation of the game DIVE, from https://alexfink.github.io/dive and an associated expectimax AI.  The AI uses a lookahead.  Before committing to any move, it will calculate all possible board positions after some number of moves and spawns, and use that to inform its committed move.  By default, the AI will start out considering board states which come from making two moves, but without considering newly spawned tiles at all.  This is referred to as depth 0.  Once it breaks 6500 points, it increases to depth 1, which considers one move, one random spawn, and then two moves.  Once it breaks 250000 points, it increases to depth 2, which considers move, spawn, move, spawn, move, move.  The combinatorial explosion of possibility means an increase in depth is a large increase in complexity, and these values have been tuned to minimize the mean computational time between completing million point games.

Usage: `./diveAI [-n ngames] [-d depth] [-s seed] [-v] [-r]`

ngames: the AI will play games until it has completed this many games.  The implication for runtime depends strongly on the other arguments.  Default is 100 games.

depth: if a depth is given as an argument, the 0, 1, 2 become max(0, depth), max(1, depth), max(2, depth) respectively.  Running games at depth 2 from the start makes for a speed which is comfortable to view, and these games generally average over 10000 points.

seed: The default seed is time(NULL), but one can set a seed for the random number generator for determinstic play.

The v flag will cause the game to print the board to the terminal after every move.  At depth 2 and above this can be seen well.

The r flag allows the AI the option to reset a game if certain criteria are not met.  The feature is currently in testing, and the criteria being used are "game reaches 25000 points without ever having more than 12 tiles on the board".  The idea is to save on processing time by not playing the end of hopeless games.  Note that ngames runs until a number of games have completed, so this flag makes the runtime significantly longer by cutting the completion ratio to 0.1% or similar.

A summary of game statistics displays while games are running.

The game will save a replay for any games exceeding 5 million points.  This replay can be viewed using `./replay "filename"`, and plays back with 0.4 seconds between moves.

Code is public under MIT public license
