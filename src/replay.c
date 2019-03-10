/* The following is needed for a `Sleep()` implementation.
 * `_DEFAULT_SOURCE` has to be defined before any system include's */

#ifdef  _WIN32
# include <windows.h>
#else /* unix */
# define _DEFAULT_SOURCE
# include <unistd.h>
# define Sleep(x) usleep((x)*1000)
#endif

#include <stdio.h>
#include <string.h>

#include "dive.h"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s filename.txt", argv[0]);
		return 0;
	}
	diveState game = {{0}, {2}, 1, 0, 2, 2, 0, 16, false};
	diveState *options;
	char buffer[10];
	uint32_t move;
	uint32_t spawn;
	uint32_t numOptions;

	FILE *f = fopen(argv[1], "r");
	char traj[60];
	strcpy(traj, "traj ");
	strcat(traj, argv[1]);
	strcat(traj, ".csv");
	//FILE *q = fopen(traj, "w");

	options = spawnOptions(game, &numOptions);
	fgets(buffer, 10, f);
	spawn = atoi(buffer);
	game = options[spawn];
	options = spawnOptions(game, &numOptions);
	fgets(buffer, 10, f);
	spawn = atoi(buffer);
	game = options[spawn];
	printf("\n\n\n\n\n\n\n\n\n");
	printBoard(game);

	while(fgets(buffer, 10, f) != NULL)
	{
		move = atoi(buffer);
		fgets(buffer, 10, f);
		spawn = atoi(buffer);
		shift(&game, (dirType) move);
		options = spawnOptions(game, &numOptions);
		if (spawn > numOptions)
			return 1;
		game = options[spawn];
		//fprintf(q, "%d,%d\n", game.numSeeds, game.emptyTiles);
		printBoard(game);
		Sleep(400);
	}
	fclose(f);
}
