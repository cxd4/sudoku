#define _CRT_SECURE_NO_WARNINGS
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

        test = 0; /* Visual Studio thinks is left "potentially uninitialized" */
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
    x = is_valid_Sudoku();
    if (x == 0)
    {
        error_freeze("Invalid Sudoku puzzle.");
        return 1;
    }
    if (x == -1)
    {
        printf("Potentially valid Jigsaw Sudoku puzzle?\n");
        error_freeze("Invalid Sudoku puzzle.");
        return 1;
    }
    y = count_unknown_squares();

    initialize_possibilities();
    show_puzzle_status();
    t1 = clock();
update_answer_log:
    log_puzzle_status();
retry_with_desperation:
    if (iterate_diagram() != 0)
        goto update_answer_log;
    if (iterate_diagram_uniquity() != 0)
        goto update_answer_log;
    desperate ^= unsolved_Sudoku();
    error_factor += desperate;
    if (desperate)
        goto retry_with_desperation;
    t2 = clock();
    printf("\n");
    delta = (float)(t2 - t1) / CLOCKS_PER_SEC;
    printf("Calculation time:  %.3f seconds\n", delta);
    delta = (float)(error_factor) / (float)(y);
    printf("Skepticism ratio:  %i / %i (%f%%)\n", error_factor, y, 100.F*delta);
    printf("Finished solving process.  Check \"answer.txt\".\n");
    return 0;
}
