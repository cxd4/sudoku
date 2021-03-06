#include <stdio.h>
#include <stdlib.h>

#define DEBUG
#undef DEBUG

#define REGION_WIDTH    3
#define PUZZLE_DEPTH    ((REGION_WIDTH) * (REGION_WIDTH))
#define TABLEMAPSIZE    64

#if (PUZZLE_DEPTH > TABLEMAPSIZE)
#error Puzzle size too large for character code page size.
#endif

#ifdef _MSC_VER
#define INLINE      __inline
#define NOINLINE    __declspec(noinline)
#elif defined(__GNUC__)
#define INLINE      inline
#define NOINLINE    __attribute__((noinline))
#else
#define INLINE
#define NOINLINE
#endif

static int desperate = 0;
/*
 * Here, "desperate" just means, "Try harder to eliminate more possibilities
 * for at least one square," until one possibility is successfully removed.
 *
 * This raises the question, "Why make 'desperate' mode OPTIONAL?"
 * Because these extra strategies risk solving the puzzle incorrectly IF,
 * and only if, the puzzle is not a true Sudoku puzzle (such as the
 * rebellious "Jigsaw" Sudoku variant, with non-3x3 box sub-grids).
 */
static int error_factor = 0;
/*
 * The number of times `desperate` mode was required to continue with the
 * solution of the puzzle (thus, the chance that the found solution might
 * not be the correct Sudoku solution for other, special variations of it).
 */

static FILE * logger;
static int last_found_square[2] = { /* (x, y) = (ptr[0], ptr[1]) */
    -1, -1 /* initialized to prevent first-time logger from treating as found */
};
static int puzzle[PUZZLE_DEPTH][PUZZLE_DEPTH];
static int possibilities[PUZZLE_DEPTH][PUZZLE_DEPTH][PUZZLE_DEPTH];
const char out_map[TABLEMAPSIZE] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', /* enough for 9x9 */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', /* Dell's code page for 16x16 grids */

    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', '0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
    'z', '=', '$' /* my attempt at supporting 64x64 grids */
};

static int is_valid_Sudoku(void);
static int unsolved_Sudoku(void);
static int count_unknown_squares(void);
static void initialize_possibilities(void);
static int is_valid_move(int x, int y, int test);
static int extract_possibility(int x, int y);
static void show_puzzle_status(void);
static void clear_puzzle_log(void);
static void log_puzzle_status(void);

static int selective_brute_force(void);
NOINLINE static int iterate_diagram(void);
static int horizontal_test(int y);
static int vertical_test(int x);
static int sub_grid_test(int x, int y);
NOINLINE static int iterate_diagram_uniquity(void);
static int horizontal_uniquity_test(int y);
static int vertical_uniquity_test(int x);
static int box_uniquity_test(int x, int y);

static int is_valid_Sudoku(void)
{
    register int x, y;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        if (horizontal_test(y) == 0)
            return 0;
    for (x = 0; x < PUZZLE_DEPTH; x++)
        if (vertical_test(x) == 0)
            return 0;
#ifndef JIGSAW_SUDOKU
    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            if (puzzle[y][x] == 0)
                continue; /* Speed up search by assuming unknowns as valid. */
            else
                if (sub_grid_test(x, y) == 0)
                    return -1;
#endif
    return 1;
}

static int unsolved_Sudoku(void)
{
#ifdef DEBUG
    return (count_unknown_squares() != 0);
#else
    register int x, y;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            if (puzzle[y][x] == 0)
                return 1;
    return 0;
#endif
}

static int count_unknown_squares(void)
{
    register int x, y;
    register int unsolved;

    unsolved = 0;
    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            unsolved += (puzzle[y][x] == 0);
    return (unsolved);
}

static void initialize_possibilities(void)
{
    register int i;
    register int x, y;

/*
 * Begin solving the puzzle assuming NOTHING.
 * Assume the elimination of no possibilities; mark all digits as possible.
 */
    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            for (i = 0; i < PUZZLE_DEPTH; i++)
                possibilities[y][x][i] = i + 1;

/*
 * Exception:  given squares already supplied by the end user.
 * Erase all possibilities (except for one, of course) for given squares.
 */
    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            if (puzzle[y][x] != 0)
            {
                for (i = 0; i < PUZZLE_DEPTH; i++)
                    possibilities[y][x][i] = 0;
                possibilities[y][x][puzzle[y][x] - 1] = puzzle[y][x];
            }
    return;
}

/*
 * This function is called only when exactly ONE possibility for a square in
 * the puzzle has been found.  It then signals the writeback of that value.
 */
INLINE static int extract_possibility(int x, int y)
{
    register int i;

    for (i = 0; i < PUZZLE_DEPTH; i++)
    {
        if (possibilities[y][x][i] == 0)
            continue;
#ifdef DEBUG
        return (i + 1);
#else
        return (possibilities[y][x][i]);
#endif
    }
    error_freeze("Tried to read square solution when none existed.");
    return 0;
}

static int iterate_diagram(void)
{
    register int x, y;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
        {
            int options;
            int test;

            if (puzzle[y][x] != 0)
                continue;
            for (test = PUZZLE_DEPTH; test != 0; --test)
                is_valid_move(x, y, test);
            options = 0;
            for (test = PUZZLE_DEPTH; test != 0; --test)
                options += (possibilities[y][x][test - 1] != 0);
            if (options == 1)
            {
                puzzle[y][x] = extract_possibility(x, y);
                last_found_square[0] = x;
                last_found_square[1] = y;
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
    return 0;
}

static int iterate_diagram_uniquity(void) /* same thing in elimination mode */
{
    register int x, y;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        if (horizontal_uniquity_test(y) != 0)
            return 1;
    for (x = 0; x < PUZZLE_DEPTH; x++)
        if (vertical_uniquity_test(x) != 0)
            return 1;

    if (desperate == 0)
        return 0;
    for (y = 0; y < REGION_WIDTH; y++)
        for (x = 0; x < REGION_WIDTH; x++)
            if (box_uniquity_test(x, y) != 0)
                return 1;
    return 0;
}

static int is_valid_move(int x, int y, int test)
{
    int pass;

#ifdef DEBUG
    if (puzzle[y][x] != 0)
        error_freeze("Testing square already filled with a value.");
#endif
    if (possibilities[y][x][test - 1] == 0)
        return 0; /* already determined by calling this before */
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

    if (desperate == 0)
        goto PASS; /* Try to delay using the 3x3 "box" tests. */
    pass = sub_grid_test(x, y);
    if (pass == 0)
        goto FAIL;
PASS:
    puzzle[y][x] = 0;
    return 1;
FAIL:
    possibilities[y][x][test - 1] = 0;
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

INLINE static void clear_puzzle_log(void)
{
    logger = fopen("answer.txt", "w");
    fclose(logger);
    logger = fopen("answer.txt", "a");
    return;
}

static void log_puzzle_status(void)
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
        if (y == last_found_square[1])
            fprintf(logger, "%s <-- put a '%c' in this row\n",
                output, output[2*last_found_square[0]]);
        else
            fprintf(logger, "%s\n", output);
    }
    fputc('\n', logger);
    return;
}

static int horizontal_test(int y)
{
    register int x;
    int counts[PUZZLE_DEPTH] = { 0 };

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
    register int y;
    int counts[PUZZLE_DEPTH] = { 0 };

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
    register int i, j;
    int counts[PUZZLE_DEPTH] = { 0 };

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
                ++counts[puzzle[y + i][x + j] - 1];
    for (i = 0; i < PUZZLE_DEPTH; i++)
        if (counts[i] > 1)
            return 0;
    return 1;
}
static int horizontal_uniquity_test(int y)
{
    register int x, i;
    int counts[PUZZLE_DEPTH] = { 0 };

    for (x = 0; x < PUZZLE_DEPTH; x++)
        for (i = 0; i < PUZZLE_DEPTH; i++)
            counts[i] += (possibilities[y][x][i] == i + 1);
#ifdef DEBUG
    for (i = 0; i < PUZZLE_DEPTH; i++)
        if (counts[i] == 0)
            error_freeze("A digit was invalid across a whole row.");
#endif
    for (i = 0; i < PUZZLE_DEPTH; i++)
    {
        register int j;

        if (counts[i] != 1)
            continue;
        for (x = 0; possibilities[y][x][i] != i + 1; x++);
        if (puzzle[y][x] != 0)
            continue;
        puzzle[y][x] = i + 1;
        last_found_square[0] = x;
        last_found_square[1] = y;
        for (j = 0; j < PUZZLE_DEPTH; j++)
            possibilities[y][x][j] = 0;
        possibilities[y][x][i] = puzzle[y][x];
        return 1;
    }
    return 0;
}
static int vertical_uniquity_test(int x)
{
    register int y, i;
    int counts[PUZZLE_DEPTH] = { 0 };

    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (i = 0; i < PUZZLE_DEPTH; i++)
            counts[i] += (possibilities[y][x][i] == i + 1);
#ifdef DEBUG
    for (i = 0; i < PUZZLE_DEPTH; i++)
        if (counts[i] == 0)
            error_freeze("A digit was invalid across a whole row.");
#endif
    for (i = 0; i < PUZZLE_DEPTH; i++)
    {
        register int j;

        if (counts[i] != 1)
            continue;
        for (y = 0; possibilities[y][x][i] != i + 1; y++);
        if (puzzle[y][x] != 0)
            continue;
        puzzle[y][x] = i + 1;
        last_found_square[0] = x;
        last_found_square[1] = y;
        for (j = 0; j < PUZZLE_DEPTH; j++)
            possibilities[y][x][j] = 0;
        possibilities[y][x][i] = puzzle[y][x];
        return 1;
    }
    return 0;
}
static int box_uniquity_test(int x, int y)
{
    register int i, j, count;
    int counts[PUZZLE_DEPTH] = { 0 };

    i = 0; /* Visual Studio thought this was "potentially uninitialized". */
/*
 * x and y should already have been multiples of REGION_WIDTH upon calling
 * the function, but there's no serious harm in making sure.
 */
    x = REGION_WIDTH * (x % REGION_WIDTH);
    y = REGION_WIDTH * (y % REGION_WIDTH);

    for (j = 0; j < REGION_WIDTH; j++)
        for (i = 0; i < REGION_WIDTH; i++)
            for (count = 0; count < PUZZLE_DEPTH; count++)
                if (possibilities[y + j][x + i][count] != count + 1)
                    continue;
                else
                    ++counts[count];
#ifdef DEBUG
    for (count = 0; count < PUZZLE_DEPTH; count++)
        if (counts[count] == 0)
            error_freeze("A digit was invalid throughout an entire box.");
#endif
    for (count = 0; count < PUZZLE_DEPTH; count++)
    {
        if (counts[count] != 1)
            continue;
        for (j = 0; j < REGION_WIDTH; j++)
            for (i = 0; i < REGION_WIDTH; i++)
                if (possibilities[y + j][x + i][count] == count + 1)
                    goto breakpoint;
breakpoint:
        if (puzzle[y + j][x + i] != 0)
            continue;
        x += i;
        y += j;
        puzzle[y][x] = count + 1;
        last_found_square[0] = x;
        last_found_square[1] = y;
        for (i = 0; i < PUZZLE_DEPTH; i++)
            possibilities[y][x][i] = 0;
        possibilities[y][x][count] = puzzle[y][x];
        return 1;
    }
    return 0;
}

static int selective_brute_force(void)
{
    int option1, option2;
    register int x, y, i, options;

    for (y = 0; y < PUZZLE_DEPTH; y++)
        for (x = 0; x < PUZZLE_DEPTH; x++)
            if (puzzle[y][x] != 0)
                continue;
            else
            {
                for (options = 0, i = 0; i < PUZZLE_DEPTH; i++)
                    if (possibilities[y][x][i] == 0)
                        continue;
                    else
                        ++options;
                if (options == 2)
                    goto attempt_brute_force;
            }
    return 0;
attempt_brute_force:
    printf("Attempting pseudo-brute-force on (x, y) = (%i, %i)...\n", x, y);
    for (i = 0; possibilities[y][x][i] == 0; i++);
    option1 = possibilities[y][x][i];
    while (possibilities[y][x][++i] == 0);
    option2 = possibilities[y][x][i];
    if (rand() & 1)
        goto pass_first_option;
    else
        goto pass_second_option;
pass_first_option:
    last_found_square[0] = x;
    last_found_square[1] = y;
    i = -1;
    while (possibilities[y][x][++i] == 0);
    puzzle[y][x] = possibilities[y][x][i];
    while (++i < PUZZLE_DEPTH)
        possibilities[y][x][i] = 0;
    return 1;
pass_second_option:
    last_found_square[0] = x;
    last_found_square[1] = y;
    i = PUZZLE_DEPTH;
    while (possibilities[y][x][--i] == 0);
    puzzle[y][x] = possibilities[y][x][i];
    while (--i >= 0)
        possibilities[y][x][i] = 0;
    return 1;
}

NOINLINE void error_freeze(const char * msg)
{
    printf("Error:  %s\n", msg);
    printf("Press ENTER to return.\n");
    getchar();
    return;
}
