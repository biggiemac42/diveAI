/* DIVE AI */
/* Brett Berger 19/12/2017
 * important contributions from "Grimy" and "12345ieee" via discord
 * Credit for this game to Alex Fink and the credits he gives
 * at https://alexfink.github.io/dive/
 */

#include "AI.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <time.h>


int main(int argc, char **argv)
{
	uint32_t ngames = 100;
	uint32_t depth = 0;
	uint32_t seed = time(NULL);
	bool verbose = false;
	bool canReset = false;

	char opt;

	while ((opt=getopt(argc,argv,"n:d:s:vrh"))!=-1)
	{
        switch (opt)
        {
            case 'n': // Num games
                ngames = atoi(optarg);
            break;
            case 'd': // AI depth
                depth = atoi(optarg);
            break;
            case 's': // Seed
                seed = atoi(optarg);
            break;
            case 'v': // Verbosity
                verbose = true;
            break;
            case 'r': // Allowed to reset
            	canReset = true;
            break;
            case 'h':
            	printf("Usage: %s [-n ngames] [-d depth] [-s seed] [-v] [-r]\n", argv[0]);
            	return 0;
            case '?':
                printf("Usage: %s [-n ngames] [-d depth] [-s seed] [-v] [-r]\n", argv[0]);
                return 1;
            default:
                return 0;
		}
	}

	uint32_t updateInterval = 1;

	srand(seed);
	uint64_t totalScore = 0;
	uint32_t aiHighScore = 0;
	uint32_t *summary;
	uint32_t score;
	uint32_t nthMove;
	uint32_t nResets;




	printf("Generating lookup table...\n");

	populateHelpList(); // Greatly speeds up future eval calls;

	printf("\033[%dA\r", 1);
	if (canReset)
		printf("Running games with resets until %d completed... \n\n\n\n", ngames);
	else
		printf("Running %d games...           \n\n\n", ngames);

	if (verbose)
		printf("\n\n\n\n\n\n\n\n");


	//FILE *q = fopen("movescore.txt", "w");
	
	for (int g = 0; g < ngames;)
	{
		summary = playGame(&score, &nthMove, depth, verbose, &nResets, canReset);

		totalScore += score;
		aiHighScore = (aiHighScore > score) ? aiHighScore : score;

		if (score > 5000000) // Print 5 million + point games to file by default
		{
			char filename[32];
			sprintf(filename, "Game%d.txt", score);
			FILE *f = fopen(filename, "w");
			for (uint32_t i = 0; i < nthMove; ++i)
				fprintf(f, "%d\n", summary[i]);
			fclose(f);
		}

		++g;

		if (!verbose && g % updateInterval == 0)
		{
			printf("\033[%dA\r", canReset ? 3 : 2);
			printf("Mean: %lu                \n", totalScore / g);
			printf("Highest: %u   \n", aiHighScore);
			if (canReset)
				printf("Completed: %d / %u (%.1f%%)\n", g, nResets + g, (double) g / (nResets + g) * 100);
		}

		free(summary);

	}

	if (verbose)
	{
		printf("Summary:\n");
		printf("Mean: %lu                \n", totalScore / ngames);
		printf("Highest: %u   \n", aiHighScore);
		if (canReset)
			printf("Completed: %d / %u (%.1f%%)\n", ngames, nResets + ngames, (double) ngames / (nResets + ngames) * 100);
	}

	//fclose(q);


	return 0;
}
