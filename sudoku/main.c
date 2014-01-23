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
    log_puzzle_status();
    show_puzzle_status();
    while (iterate_diagram());
    printf("Finished solving process.  Check \"answer.txt\".\n");
    return 0;
}
