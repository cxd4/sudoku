#include <stdio.h>

#define DEBUG
#undef DEBUG

#define REGION_WIDTH    3
#define PUZZLE_DEPTH    ((REGION_WIDTH) * (REGION_WIDTH))
#define MAX_ELEMENT     ((PUZZLE_DEPTH) - 1)
#define TABLEMAPSIZE    64

#if (PUZZLE_DEPTH > TABLEMAPSIZE)
#error Puzzle size too large for character code page size.
#endif

static int puzzle[PUZZLE_DEPTH][PUZZLE_DEPTH];
static int possibilities[PUZZLE_DEPTH];
const int out_map[TABLEMAPSIZE] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', /* enough for 9x9 */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', /* Dell's code page for 16x16 grids */

    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '-', '/' /* my attempt at supporting 64x64 grids */
};

static int iterate_diagram(void);
static void zero_possibilities(void);
static int extract_possibility(void);
static int is_valid_setup(int x, int y, int test);
static void show_puzzle_status(void);
static void log_puzzle_status(void);

static int horizontal_test(int y);
static int vertical_test(int x);
static int sub_grid_test(int x, int y);

static void zero_possibilities(void)
{
    register int i;

    for (i = 0; i < PUZZLE_DEPTH; i++)
        possibilities[i] = 0;
    return;
}

/*
 * This function is called only when exactly ONE possibility for a square in
 * the puzzle has been found.  It then signals the writeback of that value.
 */
static int extract_possibility(void)
{
    register int i;

    for (i = 0; i < PUZZLE_DEPTH; i++)
    {
        if (possibilities[i] == 0)
            continue;
#ifdef DEBUG
        return (i + 1);
#else
        return (possibilities[i]);
#endif
    }
    error_freeze("Tried to read square solution when none existed.");
    return 0;
}

static int iterate_diagram(void)
{
    register int x, y;

    for (y = MAX_ELEMENT; y >= 0; --y)
    {
        for (x = MAX_ELEMENT; x >= 0; --x)
        {
            int options;
            int test;

            if (puzzle[y][x] != 0)
                continue;
            zero_possibilities();
            options = 0;
            for (test = PUZZLE_DEPTH; test != 0; --test)
                options += is_valid_setup(x, y, test);
            if (options == 1)
            {
                puzzle[y][x] = extract_possibility();
                log_puzzle_status();
#ifdef DEBUG
                return (puzzle[y][x] != 0);
#else
                return 1;
#endif
            }
#ifdef DEBUG
            else if (options == 0)
                error_freeze("Found a square with no solutions.");
#endif
        }
    }
/*
 * More techniques to eliminate extra possibilities need to be implemented.
 */
    return 0;
}

static int is_valid_setup(int x, int y, int test)
{
    int pass;

#ifdef DEBUG
    if (puzzle[y][x] != 0)
        error_freeze("Testing square already filled with a value.");
#endif
    puzzle[y][x] = test;

/*
 * Logically, in order to minimize the number of possibilities for each blank
 * square, we want to enforce as many tests as possible to define mismatches.
 */
    pass = horizontal_test(y);
    if (pass == 0)
        goto FAIL;
    pass = vertical_test(x);
    if (pass == 0)
        goto FAIL;
    pass = sub_grid_test(x, y);
    if (pass == 0)
        goto FAIL;
    possibilities[test - 1] = test;
    puzzle[y][x] = 0;
    return 1;
FAIL:
    possibilities[test - 1] = 0;
    puzzle[y][x] = 0;
    return 0;
}

static void show_puzzle_status(void)
{
    register int x, y;
    char output[2*PUZZLE_DEPTH];

    for (y = 0; y < PUZZLE_DEPTH; y++)
    {
        for (x = 0; x < PUZZLE_DEPTH; x++)
        {
            if (puzzle[y][x] == 0)
                output[2*x] = '.';
            else
                output[2*x] = out_map[puzzle[y][x] - 1];
            output[2*x + 1] = ' ';
        }
        output[2*x - 1] = '\0';
        printf("%s\n", output);
    }
    return;
}

static void log_puzzle_status(void)
{
    FILE* stream;
    register int x, y;
    char output[2*PUZZLE_DEPTH];

    stream = fopen("answer.txt", "a");
    for (y = 0; y < PUZZLE_DEPTH; y++)
    {
        for (x = 0; x < PUZZLE_DEPTH; x++)
        {
            if (puzzle[y][x] == 0)
                output[2*x] = '.';
            else
                output[2*x] = out_map[puzzle[y][x] - 1];
            output[2*x + 1] = ' ';
        }
        output[2*x - 1] = '\0';
        fprintf(stream, "%s\n", output);
    }
    fputc('\n', stream);
    fclose(stream);
    return;
}

static int horizontal_test(int y)
{
    int counts[PUZZLE_DEPTH] = { };
    register int x;

    for (x = 0; x < PUZZLE_DEPTH; x++)
        if (puzzle[y][x] == 0)
            continue;
        else
            ++counts[puzzle[y][x] - 1];
    for (x = 0; x < PUZZLE_DEPTH; x++)
        if (counts[x] > 1)
            return 0;
    return 1;
}
static int vertical_test(int x)
{
    int counts[PUZZLE_DEPTH] = { };
    register int y;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        if (puzzle[y][x] == 0)
            continue;
        else
            ++counts[puzzle[y][x] - 1];
    for (y = 0; y < PUZZLE_DEPTH; y++)
        if (counts[y] > 1)
            return 0;
    return 1;
}
static int sub_grid_test(int x, int y)
{
    int counts[REGION_WIDTH][REGION_WIDTH] = { };
    register int i, j;
/*
 * Align the grid coordinates to the upper-left of their parent sub-grid.
 */
    x -= x % REGION_WIDTH;
    y -= y % REGION_WIDTH;

    for (i = 0; i < REGION_WIDTH; i++)
        for (j = 0; j < REGION_WIDTH; j++)
            if (puzzle[y + i][x + j] == 0)
                continue;
            else
                ++counts[i][j];
    for (i = 0; i < REGION_WIDTH; i++)
        for (j = 0; j < REGION_WIDTH; j++)
            if (counts[i][j] > 1)
                return 0;
    return 1;
}

#ifdef _MSC_VER
#define NOINLINE    __declspec(noinline)
#elif defined(__GNUC__)
#define NOINLINE    __attribute__((noinline))
#else
#define NOINLINE
#endif
NOINLINE void error_freeze(const char * msg)
{
    printf("Error:  %s\n", msg);
    printf("Press Enter to RETURN.\n");
    getchar();
    return;
}
