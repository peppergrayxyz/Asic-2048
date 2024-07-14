
/* emulator to develop and debug the game logic  */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>

#include <termios.h>
#include <unistd.h>

/* === SETTINGS ==== */

/*
    debug settings
    0 disable
    1 enabled
*/

// call debug functions and ext
#define DEBUG 0
// detailed messages in case of error
#define DEBUG_MOVE 0
// detailed messages in case of error
#define DEBUG_MOVE_REF 0
// detailed messages during progress
#define DEBUG_SEARCH 0

/*
    Search for variants to be used as test cases
    0 disable
    1 enabled

    generates random values and checks if they lead to new variants
 */
#define SEARCH 0

// number of search steps
#define NUM_SEARCH UINT32_MAX

/* Select which implementation to use for moving tiles
   0 refernce (hardcoded)
   1 v1
   2 v2
*/
#define MOVE_ALGO 2

/* Select which implementation to use for spawning tiles
   0 manual
   1 time random
   2 linear feedback shift register 
*/
#define SPAWN 1    


/* === DEFINITIONS ==== */

/* this is not really configurable */

// number of directions
#define NUM_DIRS  4
// widht of the board (x = y = widht)
#define NUM_BOARD 4
// number of fields on the bord
#define NUM_FIELD (NUM_BOARD * NUM_BOARD)
// number of fields to display the score
#define NUM_SCORE 7
// number of different signs on the board
#define NUM_SIGNS 18

/* === plattform functions ==== */

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= (unsigned) ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}
#define GETCH() getch()
#define CLEAR() system("clear");

/* === globals ==== */

// move directions
typedef enum move_direction_en {
    MV_UP = 1,
    MV_DOWN = 2,
    MV_LEFT = 3,
    MV_RIGHT = 4,
} move_direction_t;

char moveNames[5][9] = 
{
    "",
    "MV_UP   ", 
    "MV_DOWN ", 
    "MV_LEFT ", 
    "MV_RIGHT"
};

static char moveLabels[][4] = { " ", "↑", "↓", "←", "→" };

// board is stored as chars
static char field[NUM_FIELD + 1] = "                ";
static char signs[] = {' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'x'};
// score is stored as chars (base 10)
static char score[NUM_SCORE + 1] = "      0";

// last move
static move_direction_t lastMove = 0;

// last sign spawned
static int lastSpawn = 0;
// last field of board accessed by memory function
static int fieldIndex = 0;
// num shifts through memory
static int numIterations = 0;
// num iterations thorugh move logic
static int numSteps = 0;

//  linear feedback shift register init value
static uint16_t rng_value = 0x8988;

// output debug info (is set in case of error)
bool debug = false;

/* store variant seen in any lane an

    v       values as seen in one lane
    num     total numbers of variants stored

 */
typedef struct variant_store_st
{
    int num;
    bool v[5][5][5][5];
} variant_store_t;

variant_store_t testVariants = { 0, {{{{false}}}}};

// variations data of last move from reference 
static int moveRefLastValues[NUM_FIELD] = {0};


/* === MEMORY FUNCTIONS ==== */

    /*
        Memory is organized as shift register (16x8bit)
    
                        ╔═══╦═══╦═══╦═══╗
            <-----------║ f ║ e ║ d ║ c ║-----.
                        ╟───╫───╫───╫───╢     |
                    .-------------------------´
                    |   ╟───╫───╫───╫───╢
                    `---║ b ║ a ║ 9 ║ 8 ║-----.
                        ╟───╫───╫───╫───╢     |
                    .-------------------------´
                    |   ╟───╫───╫───╫───╢
                    `---║ 7 ║ 6 ║ 5 ║ 4 ║-----.
                        ╟───╫───╫───╫───╢     |
                    .-------------------------´
                    |   ╟───╫───╫───╫───╢
                    `---║ 3 ║ 2 ║ 1 ║ 0 ║-----.
                        ╚═══╩═══╩═══╩═══╝     |
            >---------------------------------´

        Data is evaluated in lanes starting with the position clostest to the edge in the moving direction
    */

/*
    computes the index for a given lane, position and moving direction
*/ 
int computeIndex(int lane, int pos, int dir)
{   

    switch(dir)
    {
        /* 
            lane    pos 0 1 2 3

              0         f e d c
              1         b a 9 8
              2         7 6 5 4
              3         3 2 1 0
         */
        case MV_LEFT: return (lane * NUM_BOARD) + (NUM_BOARD - pos - 1);

        /* 
            lane    pos 3 2 1 0

              0         f e d c
              1         b a 9 8
              2         7 6 5 4
              3         3 2 1 0
         */
        case MV_RIGHT: return (lane * NUM_BOARD) + pos;

        /* 
             pos   lane 0 1 2 3

              0         f e d c
              1         b a 9 8
              2         7 6 5 4
              3         3 2 1 0
         */
        case MV_UP: return lane + ((NUM_BOARD - pos - 1) * NUM_BOARD);

        /* 
             pos  lane  0 1 2 3

              3         f e d c
              2         b a 9 8
              1         7 6 5 4
              0         3 2 1 0
         */
        case MV_DOWN: return lane + (pos * NUM_BOARD);

    }

    assert(false);
    return -1;
}

/*
    computes the distance (number of shifts in one direction) between 2 indexes
*/
int computeMemoryDistance(int index)
{
    if(index >= fieldIndex) return index - fieldIndex;
    else                    return (NUM_FIELD - fieldIndex) + index;
}

/*
    get data from memory and update statisticall data
*/
int accessMemory(int index, bool write, int data)
{
    numIterations += computeMemoryDistance(index);

    assert(write ? data > 0 : data == 0);
    assert(index >= 0);
    assert(index < NUM_FIELD);

    fieldIndex = index;

    if(write) 
    {
        field[index] = data;
    }
    
    return field[index];
}

/*
    red the value of the last accessed index
*/
int getCurrentValue()
{
    return field[fieldIndex];
}

/*
    convert the sign to a binary number
*/
int getSignValue(char sign)
{
    int value = 0;

    for(int i = 0; i < NUM_SIGNS; i++)
    {
        if(sign == signs[i]) value = i;
    }

    return value;
}

/* === HELPER FUNCTION  ==== */

void print_game()
{
    /*
    
    ╔═══╦═══╦═══╦═══╗
    ║ f ║ e ║ d ║ c ║
    ╟───╫───╫───╫───╢
    ║ b ║ a ║ 9 ║ 8 ║
    ╟───╫───╫───╫───╢
    ║ 7 ║ 6 ║ 5 ║ 4 ║
    ╟───╫───╫───╫───╢
    ║ 3 ║ 2 ║ 1 ║ 0 ║
    ╚═══╩═══╩═══╩═══╝
    
    */

   printf("╔═══╦═══╦═══╦═══╗\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[15], field[14], field[13], field[12]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[11], field[10], field[9], field[8]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[7], field[6], field[5], field[4]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[3], field[2], field[1], field[0]);
   printf("╚═══╩═══╩═══╩═══╝\n");
}

void print_lane(int lane, int dir)
{
    printf("║ %c | %c | %c | %c ║", 
    field[computeIndex(lane, 0, dir)], 
    field[computeIndex(lane, 1, dir)], 
    field[computeIndex(lane, 2, dir)], 
    field[computeIndex(lane, 3, dir)]);
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
}

/* === GAME FUNCTIONS ==== */

/*
    spawn new tiles using time based rand
 */
void spawn_timerandom()
{
    int numSpotsLeft = 0;

    for(int i = 0; i < NUM_FIELD; i++)
    {
        if(field[i] == signs[0]) numSpotsLeft++;
    }

    if (numSpotsLeft > 0)
    {
        srand((unsigned int) time(NULL));

        // 10% chance for 4, else 2
        int value = (rand() % 10 == 0) ? 2 : 1;

        int spot = rand() % numSpotsLeft;
        int s = 0;

        for (int i = 0; i < NUM_FIELD; i++)
        {
            if (field[i] == signs[0])
            {
                if (s == spot) 
                {
                    field[i] = signs[value];
                    lastSpawn = value;
                    return;
                }

                s += 1;
            }
        }
        printf("s: %d\n", s);
    }
}

/*
    select where to spawn a new tile
 */
void spawn_manual()
{
    bool spawned = false;
    int spawn;
    bool spawn4 = false;

    printf("\rspawn?> ");

    do
    {
        int ch = GETCH();
        
        switch(ch)
        {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p': spawn = ch - 'a'; break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P': spawn = ch - 'A'; spawn4 = true; break;
            default: continue;
        }

        if(spawn < NUM_FIELD && field[spawn] == signs[0])
        {
            lastSpawn = spawn;
            spawned = true;
            field[spawn] = spawn4 ? signs[2] : signs[1];
        }

    } while(!spawned);

}

/*
    spawn new tiles using  linear feedback shift register 
 */
void spawn_tetrisrng()
{
  rng_value = ((((rng_value >> 9) & 1) ^ ((rng_value >> 1) & 1)) << 15) | (rng_value >> 1);
}

void spawn()
{
    if(SPAWN == 0) return spawn_manual();
    if(SPAWN == 1) return spawn_timerandom();
    if(SPAWN == 2) return spawn_tetrisrng();

    assert(false);
}

void addScore(int value)
{
    int currentScore = atol(score);
    int currentValue = 1 << value;
    int newScore = currentScore + currentValue;

    // printf("score: %d %d %7d\n", currentScore, currentValue, newScore);
    snprintf(score, NUM_SCORE+1, "%7d", newScore);
}

/*
    Move Function

    uses a buffer to hold a value and the current shift register value (data) to updat the field based on the move
    only two values are loaded each step

    # Evaluation #

    position    0   1   2   3 

                ╟───╫───╫───╫───╢
    lane        ║ a ║ b ║   ║   ║   <-- move direction
                ╟───╫───╫───╫───╢

              
    A. fetch                                                                        a       d       score   posBase
                                                                                    p0      p1              0                      


    B. eval                                 C. write                    D. next     a       d
                  0   1   2   3              
    2.1         ║ 0 ║ 0 ║   ║   ║                                                           p2              0
    2.2         ║ x ║ 0 ║   ║   ║                                                           p2              0
    2.3         ║ x ║ y ║   ║   ║                                                   d       p2              1
    2.4         ║ x ║ x ║   ║   ║           ║ y ║ 0 ║   ║   ║                       0       p2      +y      1
    2.5         ║ 0 ║ y ║   ║   ║           ║ y ║ 0 ║   ║   ║                       d       p2              0


    B. eval                                 C. write                    D. next     a       d
                  0   1   2   3
    2.1.1       ║ 0 ║ 0 ║ 0 ║   ║                                                           p3              0
    2.1.2       ║ 0 ║ 0 ║ x ║   ║           ║ x ║ 0 ║ 0 ║   ║                       d       p3              0

    2.2.1       ║ x ║ 0 ║ 0 ║   ║                                                           p3              0
    2.2.2       ║ x ║ 0 ║ x ║   ║           ║ y ║ 0 ║ 0 ║   ║                       d       p3      +y      1
    2.2.3       ║ x ║ 0 ║ y ║   ║           ║ x ║ y ║ 0 ║   ║                       0       p3              1
    
    2.3.1       ║ x ║ y ║ 0 ║   ║                                                           p3              1
    2.3.2       ║ x ║ y ║ x ║   ║                                                           p3              2
    2.3.3       ║ x ║ y ║ y ║   ║           ║ x ║ z ║   ║   ║                       d       p3      +z      2

    2.4.1       ║ y ║ 0 ║ 0 ║   ║                                                           p3              1
    2.4.2       ║ y ║ 0 ║ x ║   ║           ║ y ║ x ║ 0 ║   ║                       d       p3              1
    2.4.3       ║ y ║ 0 ║ y ║   ║           ║ y ║ y ║ 0 ║   ║                       0       p3              1

    2.5.1       ║ y ║ 0 ║ 0 ║   ║                                                           p3              0
    2.5.2       ║ y ║ 0 ║ x ║   ║           ║ y ║ x ║ 0 ║   ║                       d       p3              1
    2.5.3       ║ y ║ 0 ║ y ║   ║           ║ z ║ 0 ║ 0 ║   ║                       d       p3              1


    B. eval                                 C. write                                
                  0   1   2   3
    2.1.1.1     ║ 0 ║ 0 ║ 0 ║ 0 ║                                                  l1p0     l1p1           
    2.1.1.2     ║ 0 ║ 0 ║ 0 ║ x ║           ║ x ║ 0 ║ 0 ║ 0 ║         

    
    2.2.1.2     ║ x ║ 0 ║ 0 ║ x ║           ║ y ║ 0 ║ 0 ║ 0 ║                                       +y
    2.2.1.3     ║ x ║ 0 ║ 0 ║ y ║           ║ x ║ y ║ 0 ║ 0 ║         

    etc.

    == Memory Access ==        
    
    B = buff, N = next = buff + 1

                            pos      buff   data        0   1   2   3       score

                LOAD(0)     <0>              a        ║<a>║ b ║   ║   ║
                STORE        0       <a>     a        ║ a ║ b ║   ║   ║
                LOAD(1)     <1>       a     <b>       ║ a ║<b>║   ║   ║
    ------------------------------------------------------------------------------

    -->                      1        a      b        ║ a ║<b>║   ║   ║
    2.3         STORE        1       <b>     b        ║ a ║ b ║   ║   ║

    -->                      1        a      b        ║ a ║ b ║   ║   ║
    2.4         STORE_NEXT   1       <n>     b        ║ a ║ b ║   ║   ║
    2.4         CLEAR,1     <1>       n     <0>       ║ a ║<0>║   ║   ║
    2.4         WRITE,0     <0>       n     <n>       ║<n>║ 0 ║   ║   ║
    2.4         CLEAR        0       <0>     n        ║ n ║ 0 ║   ║   ║

    -->                      1        a      b        ║ a ║ b ║   ║   ║
    2.3         STORE        1        b      b        ║ a ║ 0 ║   ║   ║
    2.5         CLEAR,1     <1>       b     <0>       ║ a ║<0>║   ║   ║
    2.5         WRITE,0     <0>       b     <b>       ║<b>║ 0 ║   ║   ║



                            pos      buff   data        0   1   2   3       score

                LOAD(3)     <3>       a     <b>       ║ a ║ 0 ║ 0 ║<b>║
    ------------------------------------------------------------------------------

    -->                      3        a      b        ║ a ║   ║   ║ b ║
    2.2.1.3     STORE        3       <b>     b        ║ a ║   ║   ║<b>║

    -->                      3        a      b        ║ a ║   ║   ║ b ║
    2.2.1.2     STORE_NEXT   3       <n>     b        ║ a ║   ║   ║ b ║
    2.2.1.2     CLEAR,3     <3>       n     <0>       ║ a ║   ║   ║<0>║
    2.2.1.2     WRITE,0     <0>       n     <n>       ║<n>║   ║   ║ 0 ║
    2.2.1.2     CLEAR        3       <0>     n        ║ n ║   ║   ║ 0 ║

    -->                      3        a      b        ║ a ║   ║   ║ b ║
    2.2.1.3     STORE        3       <b>     b        ║ a ║   ║   ║<b>║
    2.2.1.3     CLEAR,3     <3>       b     <0>       ║ a ║   ║   ║<0>║
    2.2.1.3     WRITE,1     <1>       b     <b>       ║ a ║<b>║   ║   ║


*/
bool move2(move_direction_t dir)
{
    bool hasMoved = false;
    
    int buff     = signs[0];
    int data     = signs[0];
    int lane     = 0;
    int posBase  = 0;
    int posView  = 0;
    
    bool start = true;
    bool done = false;

    if(DEBUG_MOVE && debug) printf("dir: %s\n", moveLabels[dir]);
    if(DEBUG_MOVE && debug) print_game();

    do
    {       
        numSteps += 1;

        // Logic
        bool isBaseZero  = (buff == signs[0]);
        bool isViewZero  = (data == signs[0]);    
        bool eqBaseView  = (buff == data);        
        bool hasTwoTiles = !start && !isBaseZero && !isViewZero;
        bool canMerge    = hasTwoTiles && eqBaseView;
        bool hasGap      = hasTwoTiles && (posView - posBase > 1);
        bool moveTile    = !start && !isViewZero && (isBaseZero || canMerge || hasGap);

        int posBaseNext  = posBase + 1;
        int posViewNext  = posView + 1;
        int laneNext     = lane + 1;
        int incLane      = (posViewNext >= NUM_BOARD);
        done             = incLane && (laneNext >= NUM_BOARD);
        int laneBaseRead = incLane ? laneNext : lane;
        int posBaseRead  = start || incLane ? 0 : posBase;
        int posBaseWrite = !canMerge && hasGap ? posBaseNext : posBase;

        int nextLane     = incLane ? laneNext : lane;
        int nextPosBase  = incLane ? 0 : (hasTwoTiles ? posBaseNext : posBase);
        int nextPosView  = incLane ? 1 : posViewNext;
        
        // Process
        int nextValue = getSignValue(data) + 1; 
        int next      = signs[nextValue];
        if(canMerge) addScore(nextValue); 

        // Memory
        int indexClear = computeIndex(lane,         posView,      dir);
        int indexWrite = computeIndex(lane,         posBaseWrite, dir);
        int indexReadB = computeIndex(laneBaseRead, posBaseRead,  dir);
        int indexReadD = computeIndex(nextLane,     nextPosView,  dir);

        if(DEBUG_MOVE && debug) printf("[%d:%d-%d] %x '%c' '%c' ", lane, posBase, posView, fieldIndex, buff, data);
        if(DEBUG_MOVE && debug) printf("(%d%d%d%d%d%d%d) ", start, isViewZero, isBaseZero, canMerge, hasGap, hasTwoTiles,moveTile);
        if(DEBUG_MOVE && debug) print_lane(lane, dir);

        unsigned char hex[] = "0123456789abcdef";

        if(DEBUG_MOVE && debug) printf(" C %c W %c R %c %c]", 
            moveTile                    ? hex[posView]      : ' ',
            moveTile                    ? hex[posBaseWrite] : ' ',
            !done && (start || incLane) ? hex[posBaseRead]  : ' ',
            !done                       ? hex[nextPosView]  : ' ');

        if(DEBUG_MOVE && debug) printf(" ---> ");

        if(hasTwoTiles || moveTile)
        {
            buff = canMerge ? next : data;

            if(moveTile)
            {
                accessMemory(indexClear, true, signs[0]);
                accessMemory(indexWrite, true, buff);

                if(canMerge)
                {
                    addScore(nextValue);
                    buff = signs[0];
                } 
            }
        }        

        if(!done)
        {
            if(start || incLane) buff = accessMemory(indexReadB, false, 0);
            data = accessMemory(indexReadD, false, 0);            
        }

        if(DEBUG_MOVE && debug) printf("%x '%c' '%c' [%d:%d-%d] ", fieldIndex, buff, data, nextLane, nextPosBase, nextPosView);
        if(DEBUG_MOVE && debug) print_lane(lane, dir);
        if(DEBUG_MOVE && debug) printf("\n");

        start    = false;
        lane     = nextLane;
        posBase  = nextPosBase;
        posView  = nextPosView;
        hasMoved = hasMoved || moveTile;  


    } while(!done);

    if(DEBUG_MOVE && debug) print_game();

    if(hasMoved) lastMove = dir;
    
    return hasMoved;
}

bool move1(move_direction_t dir)
{
    bool hasMoved = false;
    int base = 0, data = 0;

    int lane = 0;
    int posBase = 0;
    int posData = 0;
    bool done = false;

    do
    {       
        numSteps += 1;

        int index1   = computeIndex(lane, posBase, dir);
        int index1p1 = computeIndex(lane, posBase+1, dir);
        int index2   = computeIndex(lane, posData, dir);
            
        data = accessMemory(index2, false, 0);
  
        bool start       = (posBase == 0) && (posData == 0);
        bool isBaseZero  = (base == signs[0]);
        bool isDataZero  = (data == signs[0]);    
        bool eqBaseData  = (base == data);        
        bool hasTwoTiles = !start && !isBaseZero && !isDataZero;
        bool canMerge    = hasTwoTiles && eqBaseData;
        bool hasGap      = hasTwoTiles && (posData - posBase > 1);
        bool moveTile    = !start && !isDataZero && (isBaseZero || canMerge || hasGap);

        bool incData     = canMerge;
        bool clear       = !start && !isDataZero && canMerge;
        bool load        = start || (!start && !isDataZero && !canMerge);

        int posBaseNext  = posBase + 1;
        int posDataNext  = posData + 1;
        int laneNext     = lane + 1;
        int incLane      = (posDataNext >= NUM_BOARD);
        done             = incLane && (laneNext >= NUM_BOARD);

        int nextLane     = incLane ? laneNext : lane;
        int nextPosBase  = incLane ? 0 : (hasTwoTiles ? posBaseNext : posBase);
        int nextPosData  = incLane ? 0 : posDataNext;
        
        if(!isDataZero)
        {
            if(isBaseZero)
            {
                /*
                    move
                    1   2       1     2 
                    . . 1 1 --> 1 . . 1 
                */
            }  
            else  // both not empty
            {
                if(canMerge)
                {
                    /*
                        merge
                        1 2           1 2
                        1 1 2 . --> 2 . 2 . 

                    */

                }
                else // both not empty, but can't merge
                {
                    if(hasGap) 
                    {
                        /*
                            shift value 2 to first gap spot
                            1   2         1   2
                            1 . 2 2 --> 1 2 . 2  
                        */
                    }
                    else
                    {
                        /*
                            check next value
                            1 2           1 2
                            1 2 . 2 --> 1 2 . 2
                        */
                    }
                }
            }
        }   

        assert(fieldIndex == index2);

        int nextValue = getSignValue(base) + 1;    

        if(moveTile) 
        {
                 
            int writeIndex = !canMerge && hasGap ? index1p1 : index1;
            int setData    = incData ? signs[nextValue] : data;

            int distW = computeMemoryDistance(writeIndex);
            int dist2 = computeMemoryDistance(index2);
            assert(dist2 < distW || dist2 == 0);

            accessMemory(index2, true, signs[0]);
            accessMemory(writeIndex, true, setData);
        }

        
        if(canMerge) addScore(nextValue);

        lane     = nextLane;
        posBase  = nextPosBase;
        posData  = nextPosData;
        
        if(load)  base = data;
        if(clear) base = signs[0];

        hasMoved = hasMoved || moveTile;   

    } while(!done);

    if(hasMoved) lastMove = dir;
    
    return hasMoved;
}

/*
    hardcoded move function for refernce and testing
 */
bool move_ref(int dir)
{

    if(DEBUG_MOVE_REF && debug) printf("dir: %s\n", moveLabels[dir]);
    if(DEBUG_MOVE_REF && debug) print_game();

    bool moved = false;

    int coordinates[4][4];

    if(dir == MV_LEFT)
    {
        coordinates[0][0] = 3;
        coordinates[0][1] = 2;
        coordinates[0][2] = 1;
        coordinates[0][3] = 0;
        coordinates[1][0] = 7;
        coordinates[1][1] = 6;
        coordinates[1][2] = 5;
        coordinates[1][3] = 4;
        coordinates[2][0] = 0xb;
        coordinates[2][1] = 0xa;
        coordinates[2][2] = 9;
        coordinates[2][3] = 8;
        coordinates[3][0] = 0xf;
        coordinates[3][1] = 0xe;
        coordinates[3][2] = 0xd;
        coordinates[3][3] = 0xc;
    }

    if(dir == MV_RIGHT)
    {
        coordinates[0][0] = 0;
        coordinates[0][1] = 1;
        coordinates[0][2] = 2;
        coordinates[0][3] = 3;
        coordinates[1][0] = 4;
        coordinates[1][1] = 5;
        coordinates[1][2] = 6;
        coordinates[1][3] = 7;
        coordinates[2][0] = 8;
        coordinates[2][1] = 9;
        coordinates[2][2] = 0xa;
        coordinates[2][3] = 0xb;
        coordinates[3][0] = 0xc;
        coordinates[3][1] = 0xd;
        coordinates[3][2] = 0xe;
        coordinates[3][3] = 0xf;
    }

    if(dir == MV_UP)
    {
        coordinates[0][0] = 0xc;
        coordinates[0][1] = 8;
        coordinates[0][2] = 4;
        coordinates[0][3] = 0;
        coordinates[1][0] = 0xd;
        coordinates[1][1] = 9;
        coordinates[1][2] = 5;
        coordinates[1][3] = 1;
        coordinates[2][0] = 0xe;
        coordinates[2][1] = 0xa;
        coordinates[2][2] = 6;
        coordinates[2][3] = 2;
        coordinates[3][0] = 0xf;
        coordinates[3][1] = 0xb;
        coordinates[3][2] = 7;
        coordinates[3][3] = 3;
    }

    if(dir == MV_DOWN)
    {
        coordinates[0][0] = 0;
        coordinates[0][1] = 4;
        coordinates[0][2] = 8;
        coordinates[0][3] = 0xc;
        coordinates[1][0] = 1;
        coordinates[1][1] = 5;
        coordinates[1][2] = 9;
        coordinates[1][3] = 0xd;
        coordinates[2][0] = 2;
        coordinates[2][1] = 6;
        coordinates[2][2] = 0xa;
        coordinates[2][3] = 0xe;
        coordinates[3][0] = 3;
        coordinates[3][1] = 7;
        coordinates[3][2] = 0xb;
        coordinates[3][3] = 0xf;
    }

    int convert[128] = {0};
        convert[' '] = 0;
        convert['1'] = 1;
        convert['2'] = 2;
        convert['3'] = 3;
        convert['4'] = 4;
        convert['5'] = 5;
        convert['6'] = 6;
        convert['7'] = 7;
        convert['8'] = 8;
        convert['9'] = 9;
        convert['a'] = 10;
        convert['b'] = 11;
        convert['c'] = 12;
        convert['d'] = 13;
        convert['e'] = 14;
        convert['f'] = 15;
        convert['g'] = 16;
        convert['h'] = 17;

        // only for random searching
        convert['x'] = 18;

    for(int lane = 0; lane < 4; lane++)
    {
        int indexPos1 = coordinates[lane][0];
        int indexPos2 = coordinates[lane][1];
        int indexPos3 = coordinates[lane][2];
        int indexPos4 = coordinates[lane][3];

        assert(indexPos1 < 16);
        assert(indexPos2 < 16);
        assert(indexPos3 < 16);
        assert(indexPos4 < 16);

        int data[13] = {
            0,
            convert[(int) field[indexPos1]],
            convert[(int) field[indexPos2]],
            convert[(int) field[indexPos3]],
            convert[(int) field[indexPos4]]
        };

        data[0xa] = data[1] + 1;
        data[0xb] = data[2] + 1;
        data[0xc] = data[3] + 1;

        int value1 = moveRefLastValues[indexPos1] =                                                                                                    (data[1] == 0) ? 0 : 1;
        int value2 = moveRefLastValues[indexPos2] =                                                                   (data[2] == data[1]) ? value1 : ((data[2] == 0) ? 0 : 2);
        int value3 = moveRefLastValues[indexPos3] =                                 (data[3] == data[2])  ? value2 : ((data[3] == data[1]) ? value1 : ((data[3] == 0) ? 0 : 3));
        int value4 = moveRefLastValues[indexPos4] =(data[4] == data[3]) ? value3 : ((data[4] == data[2])  ? value2 : ((data[4] == data[1]) ? value1 : ((data[4] == 0) ? 0 : 4)));

        int value = (value4 << 12) | (value3 << 8) | (value2 << 4) | (value1 << 0);
        int start = value; 

        if(DEBUG_MOVE_REF && debug) printf("%04x (%2d %2d %2d %2d) ", value, data[1], data[2], data[3], data[4]);
        if(DEBUG_MOVE_REF && debug) print_lane(lane, dir);
        if(DEBUG_MOVE_REF && debug) printf(" --> ");

        switch (value)
        {
            case 0x0000: 
            case 0x0001: 
            case 0x0021:
            case 0x0121:  
            case 0x0321:  
            case 0x1321:   
            case 0x2121:   
            case 0x2321: 
            case 0x4121:
            case 0x4321:break;

            case 0x0020: value = 0x0002; break;
            case 0x0300: value = 0x0003; break;
            case 0x4000: value = 0x0004; break;
            case 0x1001: 
            case 0x0101:
            case 0x0011: value = 0x000a; break;
            case 0x0220: 
            case 0x2020: value = 0x000b; break;
            case 0x3300: value = 0x000c; break;
            case 0x1101:
            case 0x1011: value = 0x001a; break;
            case 0x0111: value = 0x001a; break;
            case 0x2220: value = 0x002b; break;
            case 0x0301: value = 0x0031; break;
            case 0x0320: value = 0x0032; break;
            case 0x0311: value = 0x003a; break;
            case 0x4001: value = 0x0041; break;
            case 0x4020: value = 0x0042; break;
            case 0x4300: value = 0x0043; break;
            case 0x4101: 
            case 0x4011: value = 0x004a; break;
            case 0x4220: value = 0x004b; break;
            case 0x1111: value = 0x00aa; break;
            case 0x2021: value = 0x00b1; break;
            case 0x0221: value = 0x00b1; break;
            case 0x3301: value = 0x00c1; break;
            case 0x3320: value = 0x00c2; break;
            case 0x3311: value = 0x00ca; break;
            case 0x1021: value = 0x0121; break;
            case 0x1301: value = 0x0131; break;
            case 0x1311: value = 0x013a; break;
            case 0x1221: value = 0x01b1; break;
            case 0x2320: value = 0x0232; break;
            case 0x2221: value = 0x02b1; break;
            case 0x4111: value = 0x041a; break;
            case 0x4021: value = 0x0421; break;
            case 0x4301: value = 0x0431; break;
            case 0x4320: value = 0x0432; break;
            case 0x4311: value = 0x043a; break;
            case 0x4221: value = 0x04b1; break;
            case 0x1121: value = 0x0a21; break;
            case 0x3321: value = 0x0c21; break;

            default: printf("<%04x>\n", value); fflush(stdout); assert(0);
        }

        moved = moved || (start != value);

        value1 = ((0xf <<  0) & value) >>  0;
        value2 = ((0xf <<  4) & value) >>  4;
        value3 = ((0xf <<  8) & value) >>  8;
        value4 = ((0xf << 12) & value) >> 12;
        
        if(DEBUG_MOVE_REF && debug) printf("%04x (%2d %2d %2d %2d):(%2x %2x %2x %2x) ", value, data[value1], data[value2], data[value3], data[value4], 
                                                                                    signs[data[value1]], signs[data[value2]], signs[data[value3]], signs[data[value4]]);
        if(DEBUG_MOVE_REF && debug) print_lane(lane, dir);
        if(DEBUG_MOVE_REF && debug) printf("\n");


        assert(signs[data[value1]] > 0);
        assert(signs[data[value2]] > 0);
        assert(signs[data[value3]] > 0);
        assert(signs[data[value4]] > 0);

        field[indexPos1] = signs[data[value1]];
        field[indexPos2] = signs[data[value2]];
        field[indexPos3] = signs[data[value3]];
        field[indexPos4] = signs[data[value4]];

    }

    if(DEBUG_MOVE_REF && debug) print_game();

    return moved;
}

bool move(int dir)
{

    /*
        ref
     */
    if(MOVE_ALGO == 0) return move_ref(dir);

    /*
        Iterations: 11120
        Steps: 1296
    */
    if(MOVE_ALGO == 1) return move1(dir);

    /*
        Iterations: 11120
        Steps: 1053
    */
    if(MOVE_ALGO == 2) return move2(dir);

    assert(false);
}

bool canmove()
{
    /* TODO */
    return true;
}


/* === SHARED TEST CASES ==== */

struct test_fields_t
{
    char test[NUM_FIELD + 1];
    move_direction_t dir;
    char result[NUM_FIELD + 1];
    bool moved;
};

struct test_fields_t test_fields[] = {
    {"                ", MV_LEFT,  "                ", false},
    {"                ", MV_RIGHT, "                ", false},
    {"                ", MV_UP,    "                ", false},
    {"                ", MV_DOWN,  "                ", false},
    {"123456789abcdefg", MV_LEFT,  "123456789abcdefg", false},
    {"123456789abcdefg", MV_RIGHT, "123456789abcdefg", false},
    {"123456789abcdefg", MV_UP,    "123456789abcdefg", false},
    {"123456789abcdefg", MV_DOWN,  "123456789abcdefg", false},
    
    // move #8
    {"     1          ", MV_LEFT,  "       1        ", true},
    {"     1          ", MV_RIGHT, "    1           ", true},
    {"     1          ", MV_UP,    "             1  ", true},
    {"     1          ", MV_DOWN,  " 1              ", true},

    // merge 0 #12
    {" 11             ", MV_LEFT,  "   2            ", true},
    {" 11             ", MV_RIGHT, "2               ", true},
    {"     1   1      ", MV_UP,    "             2  ", true},
    {"     1   1      ", MV_DOWN,  " 2              ", true},

    // move with gap 1 #16
    {"    1 2         ", MV_LEFT,  "      12        ", true},
    {"     1 2        ", MV_RIGHT, "    12          ", true},
    {" 1       2      ", MV_UP,    "         1   2  ", true},
    {"     1       2  ", MV_DOWN,  " 1   2          ", true},

    // merge with gap 1 #20
    {"    2 2         ", MV_LEFT,  "       3        ", true},
    {"     2 2        ", MV_RIGHT, "    3           ", true},
    {" 2       2      ", MV_UP,    "             3  ", true},
    {"     2       2  ", MV_DOWN,  " 3              ", true},

    // move with gap 2 #24
    {"    2  2        ", MV_LEFT,  "       3        ", true},
    {"    2  2        ", MV_RIGHT, "    3           ", true},
    {"  1           2 ", MV_UP,    "          1   2 ", true},
    {"  1           2 ", MV_DOWN,  "  1   2         ", true},

    // merge with gap 2 #28
    {"    2  2        ", MV_LEFT,  "       3        ", true},
    {"    2  2        ", MV_RIGHT, "    3           ", true},
    {"  2           2 ", MV_UP,    "              3 ", true},
    {"  2           2 ", MV_DOWN,  "  3             ", true},

    // combined test #32
    {"3311            ", MV_LEFT,  "  42            ", true},
    {"3311            ", MV_RIGHT, "42              ", true},
    {"3   3   1   1   ", MV_UP,    "        4   2   ", true},
    {"3   3   1   1   ", MV_DOWN,  "4   2           ", true},

    // multi merge #36
    {"1111            ", MV_LEFT,  "  22            ", true},
    {"1111            ", MV_RIGHT, "22              ", true},
    {"1   1   1   1   ", MV_UP,    "        2   2   ", true},
    {"1   1   1   1   ", MV_DOWN,  "2   2           ", true},

    // shift
    {"121             ", MV_LEFT,  " 121            ", true},
    {" 121            ", MV_RIGHT, "121             ", true},
    {"1   2   1       ", MV_UP,    "    1   2   1   ", true},
    {"    1   2   1   ", MV_DOWN,  "1   2   1       ", true},

    //shift and merge
    {"111             ", MV_LEFT,  "  12            ", true},
    {"111             ", MV_RIGHT, "21              ", true},
    {"1   1   1       ", MV_UP,    "        1   2   ", true},
    {"    1   1   1   ", MV_DOWN,  "2   1           ", true},

    //edge cases *pun intended*
    {"1231123112311231", MV_LEFT,  "1231123112311231", false},
    {"1231123112311231", MV_RIGHT, "1231123112311231", false},
    {"1111222233331111", MV_UP,    "1111222233331111", false},
    {"1111222233331111", MV_DOWN,  "1111222233331111", false},


    // variants
    {"7 4efe722284g2a ", MV_LEFT,  " 74efe72 384 g2a", true }, // 4021432133214320
    {"7 4efe722284g2a ", MV_RIGHT, "74e fe72384 g2a ", true }, // 1034123411341230
    {"c2a7gf7  g2ggdgh", MV_UP,    " 2a  f77cg2ghdgh", true }, // 4444133002221111
    {"2fge5815d 6e 9gc", MV_DOWN,  "2fge5815d96e  gc", true }, // 1111222230310414
    {"b2 9h6e57gg1f3b ", MV_LEFT,  " b29h6e5 7h1 f3b", true }, // 4301432142214320
    {"d257b6bh5fc299 d", MV_RIGHT, "d257b6bh5fc2ad  ", true }, // 1234121412341104
    {"h9b6dg6d133c39f9", MV_RIGHT, "h9b6dg6d14c 39f9", true }, // 1234123112241232
    {"chdg3 1a8f 49f29", MV_DOWN , "chdg3g1a8 249  9", true }, // 1111202233034344
    {" g2 21d7b3af87g2", MV_LEFT , "  g221d7b3af87g2", true }, // 0320432143214321
    {"49g79hfgfd 6b f9", MV_DOWN , "49g79hggfd 6b  9", true }, // 1111222233034024
    {"6 16e4 bhh2hgag ", MV_LEFT , " 616 e4b x2h gag", true }, // 1021430111212320
    {"6 16e4 bhh2hgag ", MV_RIGHT, "616 e4b x2h gag ", true }, // 1031120411311210
    {"fb d629f435fcc6d", MV_UP   , "fb  629d435gcc6d", true }, // 4401333222221111
    {"8 2c aa3bf 374h7", MV_LEFT , " 82c  b3 bf374h7", true }, // 4021022143011321
    {"8 2c aa3bf 374h7", MV_RIGHT, "82c b3  bf3 74h7", true }, // 1034022412041231
    {"94f25cd21dcbach ", MV_UP   , "94f 5cd 1dc3achb", true }, // 4443313322221110
    {"94f25cd21dcbach ", MV_DOWN , "94f35cdb1dc ach ", true }, // 1111222133334240
    {"bbb1g38defahb4b4", MV_LEFT , " bc1g38defahb4b4", true }, // 2221432143212121
    {"bbb1g38defahb4b4", MV_RIGHT, "cb1 g38defahb4b4", true }, // 1114123412341212
    {"gg g 997c eae9g5", MV_LEFT , "  gh  a7 ceae9g5", true }, // 1101022140214321
    {"gg g 997c eae9g5", MV_RIGHT, "hg  a7  cea e9g5", true }, // 1101022410341234
    {"  7e96f78e1d b31", MV_LEFT , "  7e96f78e1d b31", false}, // 0021432143210321
    {"  7e96f78e1d b31", MV_RIGHT, "7e  96f78e1db31 ", true }, // 0034123412340234
    {"7 98efg65 f5668 ", MV_UP   , "7 9 e g85ff66685", true }, // 4044333320221110
    {" 2 2g4h23e7 e c1", MV_LEFT , "   3g4h2 3e7 ec1", true }, // 0101432143204021
    {"7198b4dbgecc   e", MV_LEFT , "7198b4db ged   e", true }, // 4321132143110001
    {"7198b4dbgecc   e", MV_RIGHT, "7198b4dbged e   ", true }, // 1234123112330004
    {"6bb 6 dd d73 369", MV_UP   , "  b  bdd d737369", true }, // 3440303302220111
    {"6bb 6 dd d73 369", MV_DOWN , "7bbd dd3 379  6 ", true }, // 1110102203330444
};


/* === VARIANT SEARCH ==== */

bool updateVariant(variant_store_t *variants, int lane, int dir)
{    
    bool *v = &(variants->v[moveRefLastValues[ computeIndex(lane, 0, dir)]]\
                           [moveRefLastValues[ computeIndex(lane, 1, dir)]]\
                           [moveRefLastValues[ computeIndex(lane, 2, dir)]]\
                           [moveRefLastValues[ computeIndex(lane, 3, dir)]]);

    if(!*v)
    {
        *v = true;
        variants->num++;
        return true;
    }

    return false;
}

variant_store_t* validate_move_tests(variant_store_t *variants)
{
    int numTests = sizeof(test_fields)/sizeof(test_fields[0]);
    for(int i = 0; i < numTests; i++)
    {
        int dir = test_fields[i].dir;
        strncpy(field, test_fields[i].test, NUM_FIELD); 
        move_ref(dir);

        for(int lane = 0; lane < NUM_BOARD; lane++)
        {
           updateVariant(variants, lane, dir);
        }
    }

    return variants;
}

void printLastVariante()
{
    for(int i = 0; i < NUM_FIELD; i++)
    {
        printf("%x", moveRefLastValues[i]);
    }
}

void search_variants_rnd(size_t limit)
{
    printf("\n=== search_variants_rnd ===\n");

    printf("\n Limit: %lu", limit);
    printf("\n Known Variants: %d\n\n", testVariants.num);

    int newVariants = 0;
    char test[NUM_FIELD + 1]   = "                ";

    srand((unsigned int) time(NULL));

    for(size_t i = 0; i < limit; i++)
    {
        for(int j = 0; j < NUM_FIELD; j++)
        {
            test[j] = signs[(rand() % NUM_SIGNS)];
        }

        for(int dir = 1; dir <= NUM_DIRS; dir++)
        {
            strncpy(field, test, NUM_FIELD);
            bool moved = move_ref(dir);

            bool newVariantFound = false;

            for(int lane = 0; lane < NUM_BOARD; lane++)
            {
                if(updateVariant(&testVariants, lane, dir))
                {
                    newVariantFound = true;
                }
            }

            if(newVariantFound || DEBUG_SEARCH)
            {
                newVariants++;

                printf(" {\"%s\", %s, \"%s\", %s}, // (", test, moveNames[dir], field, moved ? "true " : "false");
                printLastVariante();
                printf(")");
                if(DEBUG_SEARCH && newVariantFound) printf(" (new)");
                printf("\n");
            }
        }
    }

    printf("\n New Variants: %d", newVariants);

    printf("\n");    
}


/* === DEBUG FUNCTIONS ==== */

void debug_spawn_tetrisrng()
{
    printf("\n=== debug_spawn_tetrisrng ===\n\n");

    for(int i = 0; i < 16; i++)
    {
        printf(" ");
        printBits(sizeof(uint16_t), &rng_value); printf(" %04x\n", rng_value);
        spawn_tetrisrng();
    }
}

void debug_computeIndex()
{
    printf("\n=== debug_computeIndex ===\n");

    for(int dir = 1; dir <= 4; dir++)
    {
        printf("\n direction: %s (%d)\n", moveLabels[dir], dir);

        for(int lane = 0; lane < NUM_BOARD; lane++)
        {
            for(int pos = 0; pos < NUM_BOARD; pos++)
            {
                printf(" %d.%d (%2d)  ", lane, pos, computeIndex(lane, pos, dir));
            } 
            printf("\n");
        }
    }
}

void debug_move()
{

    printf("\n=== debug_move ===\n");

    char result[NUM_FIELD + 1] = "                ";

    debug = 0;

    int interationsTotal = 0;
    int stepsTotal = 0;

    printf("\n [ #] ✥ field             variant             St Itr\n");

    int numTests = sizeof(test_fields)/sizeof(test_fields[0]);
    for(int i = 0; i < numTests; i++)
    {
        strncpy(field, test_fields[i].test, NUM_FIELD); 
        
        if(debug) printf("\n");
        if(debug) printf("[%d] setup (%s) '%s'\n", i, moveLabels[test_fields[i].dir], field);
        if(debug) print_game();

        int moved1 = move_ref(test_fields[i].dir);
        strncpy(result, field, NUM_FIELD);
        strncpy(field, test_fields[i].test, NUM_FIELD);

        if(debug) printf("[%d] mov Ref : %d, '%s'\n", i, moved1, result);
        if(debug) print_game();

        fieldIndex = 0;
        numIterations = 0;
        numSteps = 0;
        int moved2 = move(test_fields[i].dir);
        interationsTotal += numIterations;
        stepsTotal += numSteps;

        if(debug) printf("[%d] test      %s  '%s'\n", i, moveLabels[test_fields[i].dir], field);
        if(debug) print_game();

        bool testMoved = (moved1 == moved2);
        bool testField = (strncmp(result, field, NUM_FIELD) == 0);

        if(debug) printf("[%d] test    : %s  '%s'\n", i, moveLabels[test_fields[i].dir], field);
        if(debug) printf("[%d] mov Ref : %d, '%s'\n", i, moved1, result);
        if(debug) printf("[%d] mov Test: %d, '%s'\n", i, moved2, field);
        if(debug) printf("\n");

        printf(" [%2d] %s '%s'(", i, moveLabels[test_fields[i].dir],  test_fields[i].test);
        printLastVariante();
        printf("): %2d %3d\n", numSteps, numIterations);

        if(!debug && (!testMoved || !testField))
        {
            i--;
            debug = true;
            continue;
        }

        assert(testMoved);   
        assert(testField);   
    }

    int numVariants = validate_move_tests(&testVariants)->num;

    printf("\n");
    printf(" NumTests: %d\n", numTests);
    printf(" Variants: %d\n", numVariants);
    printf(" Iterations: %d\n", interationsTotal);
    printf(" Steps: %d\n", stepsTotal);
}


/* === TEST FUNCTIONS ==== */

void test_computeIndex()
{
    printf("[test_computeIndex] ");

    strncpy(field, "123456789abcdefg", NUM_FIELD+1);

    /*
    ╔═══╦═══╦═══╦═══╗
    ║ g ║ f ║ e ║ d ║
    ╟───╫───╫───╫───╢
    ║ c ║ b ║ a ║ 9 ║
    ╟───╫───╫───╫───╢
    ║ 8 ║ 7 ║ 6 ║ 5 ║
    ╟───╫───╫───╫───╢
    ║ 4 ║ 3 ║ 2 ║ 1 ║
    ╚═══╩═══╩═══╩═══╝

    */

    assert(field[computeIndex(0, 0, MV_LEFT)] == '4');
    assert(field[computeIndex(0, 1, MV_LEFT)] == '3');
    assert(field[computeIndex(0, 2, MV_LEFT)] == '2');
    assert(field[computeIndex(0, 3, MV_LEFT)] == '1');
    assert(field[computeIndex(1, 0, MV_LEFT)] == '8');
    assert(field[computeIndex(1, 1, MV_LEFT)] == '7');
    assert(field[computeIndex(1, 2, MV_LEFT)] == '6');
    assert(field[computeIndex(1, 3, MV_LEFT)] == '5');
    assert(field[computeIndex(2, 0, MV_LEFT)] == 'c');
    assert(field[computeIndex(2, 1, MV_LEFT)] == 'b');
    assert(field[computeIndex(2, 2, MV_LEFT)] == 'a');
    assert(field[computeIndex(2, 3, MV_LEFT)] == '9');
    assert(field[computeIndex(3, 0, MV_LEFT)] == 'g');
    assert(field[computeIndex(3, 1, MV_LEFT)] == 'f');
    assert(field[computeIndex(3, 2, MV_LEFT)] == 'e');
    assert(field[computeIndex(3, 3, MV_LEFT)] == 'd');

    assert(field[computeIndex(0, 0, MV_RIGHT)] == '1');
    assert(field[computeIndex(0, 1, MV_RIGHT)] == '2');
    assert(field[computeIndex(0, 2, MV_RIGHT)] == '3');
    assert(field[computeIndex(0, 3, MV_RIGHT)] == '4');
    assert(field[computeIndex(1, 0, MV_RIGHT)] == '5');
    assert(field[computeIndex(1, 1, MV_RIGHT)] == '6');
    assert(field[computeIndex(1, 2, MV_RIGHT)] == '7');
    assert(field[computeIndex(1, 3, MV_RIGHT)] == '8');
    assert(field[computeIndex(2, 0, MV_RIGHT)] == '9');
    assert(field[computeIndex(2, 1, MV_RIGHT)] == 'a');
    assert(field[computeIndex(2, 2, MV_RIGHT)] == 'b');
    assert(field[computeIndex(2, 3, MV_RIGHT)] == 'c');
    assert(field[computeIndex(3, 0, MV_RIGHT)] == 'd');
    assert(field[computeIndex(3, 1, MV_RIGHT)] == 'e');
    assert(field[computeIndex(3, 2, MV_RIGHT)] == 'f');
    assert(field[computeIndex(3, 3, MV_RIGHT)] == 'g');

    assert(field[computeIndex(0, 0, MV_UP)] == 'd');
    assert(field[computeIndex(0, 1, MV_UP)] == '9');
    assert(field[computeIndex(0, 2, MV_UP)] == '5');
    assert(field[computeIndex(0, 3, MV_UP)] == '1');
    assert(field[computeIndex(1, 0, MV_UP)] == 'e');
    assert(field[computeIndex(1, 1, MV_UP)] == 'a');
    assert(field[computeIndex(1, 2, MV_UP)] == '6');
    assert(field[computeIndex(1, 3, MV_UP)] == '2');
    assert(field[computeIndex(2, 0, MV_UP)] == 'f');
    assert(field[computeIndex(2, 1, MV_UP)] == 'b');
    assert(field[computeIndex(2, 2, MV_UP)] == '7');
    assert(field[computeIndex(2, 3, MV_UP)] == '3');
    assert(field[computeIndex(3, 0, MV_UP)] == 'g');
    assert(field[computeIndex(3, 1, MV_UP)] == 'c');
    assert(field[computeIndex(3, 2, MV_UP)] == '8');
    assert(field[computeIndex(3, 3, MV_UP)] == '4');

    assert(field[computeIndex(0, 0, MV_DOWN)] == '1');
    assert(field[computeIndex(0, 1, MV_DOWN)] == '5');
    assert(field[computeIndex(0, 2, MV_DOWN)] == '9');
    assert(field[computeIndex(0, 3, MV_DOWN)] == 'd');
    assert(field[computeIndex(1, 0, MV_DOWN)] == '2');
    assert(field[computeIndex(1, 1, MV_DOWN)] == '6');
    assert(field[computeIndex(1, 2, MV_DOWN)] == 'a');
    assert(field[computeIndex(1, 3, MV_DOWN)] == 'e');
    assert(field[computeIndex(2, 0, MV_DOWN)] == '3');
    assert(field[computeIndex(2, 1, MV_DOWN)] == '7');
    assert(field[computeIndex(2, 2, MV_DOWN)] == 'b');
    assert(field[computeIndex(2, 3, MV_DOWN)] == 'f');
    assert(field[computeIndex(3, 0, MV_DOWN)] == '4');
    assert(field[computeIndex(3, 1, MV_DOWN)] == '8');
    assert(field[computeIndex(3, 2, MV_DOWN)] == 'c');
    assert(field[computeIndex(3, 3, MV_DOWN)] == 'g');

    printf("ok.\n");
}

void test_move()
{
    printf("[test_move] ");
    
    debug = false;

    int numTests = sizeof(test_fields)/sizeof(test_fields[0]);
    for(int i = 0; i < numTests; i++)
    {
       strncpy(field, test_fields[i].test, NUM_FIELD);

        if(debug) printf("=== %d === \n", i);
        if(debug) printf("\n"); 
        if(debug) print_game();

        bool moved = move(test_fields[i].dir);
        
        if(debug) printf("\n"); 
        if(debug) print_game();
        if(debug) printf("'%s' %s '%s' (%s): '%s' (%s) [%3d][%2d]\n", test_fields[i].test, moveLabels[test_fields[i].dir], test_fields[i].result, test_fields[i].moved ? "true " : "false", field, moved ? "true " : "false", numIterations, numSteps);
        
        bool testMoved = (test_fields[i].moved == moved);
        bool testField = (strncmp(field, test_fields[i].result, NUM_FIELD) == 0);

        if(!debug && (!testMoved || !testField))
        {
            i--;
            debug = true;
            printf("\n");
            continue;
        }

        assert(testMoved);   
        assert(testField); 
    }  

    printf("ok.\n");
}


/* === MAIN ==== */

int main()
{
    int ch;
    bool moved = true;
    size_t numMoves = 0;
  
    if(DEBUG) search_variants_rnd(1);
    if(DEBUG) debug_spawn_tetrisrng();
    if(DEBUG) debug_computeIndex();
    if(DEBUG) debug_move();  

    if(SEARCH) search_variants_rnd(NUM_SEARCH);

    if(DEBUG) printf("\n=== tests ===\n\n"); 
    test_computeIndex();
    test_move();    
    
    if(DEBUG) return 0;

    strncpy(field, "                ", NUM_FIELD + 1);
    strncpy(score, "      0", NUM_SCORE + 1);
    fieldIndex = 0; 
    numIterations = 0;
    lastMove = 0;

    do
    {
        if (moved)
        {
            CLEAR();

            numMoves++;
            spawn();

            printf("\nfield: '%s' score: %s last spawn: %d last move: %s step: %zu\n", field, score, lastSpawn, moveLabels[lastMove], numMoves);
            print_game();
            
            moved = false;
        }


        if (!canmove())
        {
            printf("\nGame Over!");
            break;
        }

        //printf("\rmove?> ");
        ch = GETCH();

        switch (ch)
        {
        case 91:
            ch = GETCH();
            switch (ch)
            {
            case 65: // up
                moved = move(MV_UP);
                break;
            case 66: // down
                moved = move(MV_DOWN);
                break;
            case 68: // left 
                moved = move(MV_LEFT);
                break;
            case 67: // right
                moved = move(MV_RIGHT);
                break;
            }
            break;
        }

    } while (ch != 'x');

    return EXIT_SUCCESS;
}
