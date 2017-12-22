# diveAI

A C reimplementation of the game DIVE, from https://alexfink.github.io/dive
and an associated expectimax AI.  The evaluation function is nontrivial, but
currently favors a single high seed, empty space, and low seed count.

Usage: `./diveAI [-n ngames] [-d depth] [-s seed] [-v]`

The program will run `ngames` games, default 10.
The AI looks ahead by `depth` random spawns and chooses the move with the highest
expected fitness.  Default depth is 2.
The game may be set to a given seed for consistent results, default seed is
`time(NULL)`.
If the verbosity flag is set, the board is printed after every move.

Code is public under MIT public license