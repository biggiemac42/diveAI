#include "dive.h"

#include <stdio.h>

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
	options = spawnOptions(game, &numOptions);
	fgets(buffer, 10, f);
	spawn = atoi(buffer);
	game = options[spawn];
	options = spawnOptions(game, &numOptions);
	fgets(buffer, 10, f);
	spawn = atoi(buffer);
	game = options[spawn];
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
		printBoard(game);
	}
	fclose(f);
}