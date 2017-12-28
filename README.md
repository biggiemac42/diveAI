# diveAI

A C reimplementation of the game DIVE, from https://alexfink.github.io/dive and an associated expectimax AI.

Usage: `./diveAI [-n ngames] [-d depth] [-s seed] [-v]`

I have changed how the game handles "depth."  Currently depth N considers the next N spawns and N + 2 moves.  Once a game reaches 10000 points it will force depth to a minimum of 1, and at 100000 points it forces minimum depth 2.  As a result, running at depth 0 will devote more processing power to the higher scoring games while not going slowly for the rest - the program runs about 3 million games in an hour at depth 0 and while the average score is around 2000, it gets million point games somewhat frequently.  Depth 0 is the default.

At depth 2 the program runs about 150 games an hour and averages over 10000 points.

The default number of games is 100.

The default seed is time(NULL), but one can set a seed for the random number generator.

The verbosity flag will cause the game to print the board to the terminal after every move.  At depth 2 and above this can be seen well, but lower depth will print too often and not be useful.

The game will save a replay for any games exceeding a million points.  This replay can be viewed using `./replay "filename"`, and plays back with 0.4 seconds between moves.

Code is public under MIT public license