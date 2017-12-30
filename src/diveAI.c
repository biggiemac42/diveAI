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
#include <time.h>

int main(int argc, char **argv)
{
	uint32_t ngames = 100;
	uint32_t depth = 0;
	uint32_t seed = time(NULL);
	bool verbose = false;

	char opt;

	while ((opt=getopt(argc,argv,"n:d:s:v"))!=-1)
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
            case '?':
                return 1;
            default:
                printf("Usage: %s [-n ngames] [-d depth] [-s seed] [-v]", argv[0]);
                return 0;
		}
	}


	srand(seed);
	uint64_t totalScore = 0;
	uint32_t aiHighScore = 0;
	uint32_t n10k = 0;
	uint32_t *summary;
	uint32_t score;
	uint32_t nthMove;

	printf("Generating lookup table...\n\n\n");

	populateHelpList(); // Greatly speeds up future eval calls;
	
	for (int g = 0; g < ngames; ++g)
	{
		summary = playGame(&score, &nthMove, depth, verbose);

		totalScore += score;
		aiHighScore = (aiHighScore > score) ? aiHighScore : score;
		if (score > 10000)
			++n10k;
		if (score > 1000000)
		{
			char filename[32];
			sprintf(filename, "Game%d.txt", score);
			FILE *f = fopen(filename, "w");
			for (uint32_t i = 0; i < nthMove; ++i)
				fprintf(f, "%d\n", summary[i]);
			fclose(f);
		}

		if (g % 10 == 9)
		{
			printf("\033[%dA\r", 3);
			printf("Mean : %lu                \n", totalScore / (g+1));
			printf("Highest: %d   \n", aiHighScore);
			printf("10K games: %d / %d (%.1f%%)\n", n10k, (g+1), (double)n10k / (g+1) * 100);
		}

		free(summary);

	}

	return 0;
}
