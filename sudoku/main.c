#include <stdio.h>
#include <time.h>

extern void error_freeze(const char* msg);

#include "sudoku.h"

int main(void)
{
    FILE* stream;
    clock_t t1, t2;
    register float delta;
    register int x, y;

    stream = fopen("puzzle.txt", "r");
    if (stream == NULL)
    {
        error_freeze("Failed to read \"puzzle.txt\".");
        return 1;
    }
    printf("Loading puzzle...\n\n");
    for (y = 0; y < PUZZLE_DEPTH; y++)
    {
        int test;
        register int i;

        for (x = 0; x < PUZZLE_DEPTH; x++)
        {
            test = fgetc(stream);
            if (test == EOF)
                goto unexpected_eof;
            for (i = 0; i < PUZZLE_DEPTH; i++)
                if (out_map[i] == (char)test)
                    break;
            puzzle[y][x] = (i < PUZZLE_DEPTH) ? i + 1 : 0;
            test = fgetc(stream); /* Skip every other character of text. */
            if (test == EOF)
                goto unexpected_eof;
        }
     /* Ignore all remaining bytes until the end of the line. */
        while (test != '\n')
        {
            test = fgetc(stream);
            if (test == EOF)
                goto unexpected_eof;
        }
        continue;
unexpected_eof:
        error_freeze("Unexpected end of puzzle text file.");
        return 1;
    }
    fclose(stream);
    clear_puzzle_log();

    initialize_possibilities();
    show_puzzle_status();
    t1 = clock();
FIRST_PASS:
    log_puzzle_status();
    if (iterate_diagram() != 0)
        goto FIRST_PASS;
    if (iterate_diagram_uniquity() != 0)
        goto FIRST_PASS;
    t2 = clock();
    printf("\n");
    delta = (float)(t2 - t1) / CLOCKS_PER_SEC;
    printf("Calculation time:  %.3f seconds\n", delta);
    printf("Finished solving process.  Check \"answer.txt\".\n");
    return 0;
}
