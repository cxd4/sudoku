#include <stdio.h>

extern void error_freeze(const char* msg);

#include "sudoku.h"

int main(void)
{
    FILE* stream;
    register int x, y;

    stream = fopen("puzzle.txt", "r");
    if (stream == NULL)
    {
        error_freeze("Failed to read \"puzzle.txt\".");
        return 1;
    }
    printf("Loading puzzle...\n");
    for (y = 0; y < REGION_WIDTH; y++)
    {
        int test;
        register int i;

        for (x = 0; x < REGION_WIDTH; x++)
        {
            test = fgetc(stream);
            if (test == EOF)
                goto unexpected_eof;
            for (i = 0; i < PUZZLE_DEPTH; i++)
                if (out_map[i] == (char)test)
                    break;
            if (i >= PUZZLE_DEPTH)
            {
                error_freeze("Invalid Sudoku digit for this puzzle size.");
                return 1;
            }
            puzzle[y][x] = i + 1;
            if (fgetc(stream) == EOF) /* Skip every other character of text. */
                goto unexpected_eof;
        }
     /* Ignore all remaining bytes until the end of the line. */
        do {
            test = fgetc(stream);
            if (test == EOF)
                goto unexpected_eof;
        } while (test != '\n');
        continue;
unexpected_eof:
        error_freeze("Unexpected end of puzzle text file.");
        return 1;
    }
    fclose(stream);
    show_puzzle_status();
    printf("Pause the solver after each step (hint mode, Y/N)?\n");
    trace_step_by_step = getchar() & 1; /* If the character is even, then no. */
    while (iterate_diagram());
    show_puzzle_status();
    return 0;
}