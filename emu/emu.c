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

typedef enum move_direction_en {
    MV_UP = 1,
    MV_DOWN = 2,
    MV_LEFT = 3,
    MV_RIGHT = 4,
} move_direction_t;


#define NUM_BOARD 4
#define NUM_FIELD (NUM_BOARD * NUM_BOARD)
#define NUM_SCORE 7
#define NUM_SIGNS 18

static move_direction_t lastMove = 0;
static int lastSpawn = 0;
static unsigned char moves[][4] = { " ", "↑", "↓", "←", "→" };
static unsigned char field[NUM_FIELD + 1] = "                ";
static int fieldIndex = 0;
static int numIterations = 0;
static int numSteps = 0;
static unsigned char score[NUM_SCORE + 1] = "      0";
static unsigned char signs[NUM_SIGNS] = {' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

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

static uint16_t rng_value = 0x8988;

void spawn_tetrisrng()
{
  rng_value = ((((rng_value >> 9) & 1) ^ ((rng_value >> 1) & 1)) << 15) | (rng_value >> 1);
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

void debug_spawn_tetrisrng()
{
    for(int i = 0; i < 16; i++)
    {
        printBits(sizeof(uint16_t), &rng_value); printf(" %x\n", rng_value);
        spawn_tetrisrng();
    }
}

void spawn()
{
    spawn_timerandom();
}


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

void print_lane(int lane, int dir)
{
    printf("║ %c | %c | %c | %c║", 
    field[computeIndex(lane, 0, dir)], 
    field[computeIndex(lane, 1, dir)], 
    field[computeIndex(lane, 2, dir)], 
    field[computeIndex(lane, 3, dir)]);
}

int computeMemoryDistance(int index)
{
    if(index >= fieldIndex) return index - fieldIndex;
    else                    return (NUM_FIELD - fieldIndex) + index;
}

int accessMemory(int index, bool write, int data)
{
    numIterations += computeMemoryDistance(index);

    fieldIndex = index;

    if(write) 
    {
        field[index] = data;
    }
    
    return field[index];
}

int getCurrentValue()
{
    return field[fieldIndex];
}

int getSignValue(int sign)
{
    int value = 0;

    for(int i = 0; i < NUM_SIGNS; i++)
    {
        if(sign == signs[i]) value = i;
    }

    return value;
}

void addScore(int value)
{
    int currentScore = atol(score);
    int currentValue = 1 << value;
    int newScore = currentScore + currentValue;

    // printf("score: %d %d %7d\n", currentScore, currentValue, newScore);
    sprintf(score, "%7d", newScore);
}

/*
Iterations: 7330
Steps: 520

Iterations: 5218
Steps: 640

    == Evaluation ==   

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
bool move(move_direction_t dir)
{
    bool hasMoved = false;
    
    int buff     = 'x';
    int data     = 'y';
    int lane     = 0;
    int posBase  = 0;
    int posView  = 0;
    
    bool start = true;
    bool done = false;

    // printf("dir: %s\n", moves[dir]);
    // print_game();

    do
    {       
        numSteps += 1;

        bool updateScore = false;

        bool isBaseZero  = (buff == signs[0]);
        bool isViewZero  = (data == signs[0]);    
        bool eqBaseView  = (buff == data);        
        bool hasTwoTiles = !start && !isBaseZero && !isViewZero;
        bool canMerge    = hasTwoTiles && eqBaseView;
        bool hasGap      = hasTwoTiles && (posView - posBase > 1);
        bool moveTile    = !start && !isViewZero && (isBaseZero || canMerge || hasGap);

        bool incView     = canMerge;
        bool clear       = !start && !isViewZero && canMerge;
        bool load        = start || (!start && !isViewZero && !canMerge);

        int posBaseNext  = posBase + 1;
        int posViewNext  = posView + 1;
        int laneNext     = lane + 1;
        int incLane      = (posViewNext >= NUM_BOARD);
        done             = incLane && (laneNext >= NUM_BOARD);
        int laneBaseRead = incLane ? laneNext : lane;
        int posBaseRead  = start || incLane ? 0 : posBase;
        int posBaseWrite = !canMerge && hasGap ? posBaseNext : posBase;

        int nextLane     = incLane ? laneNext : lane;
        int nextPosBase  = incLane ? 0 : (hasTwoTiles ? (canMerge && hasGap ? posBaseNext : posView) : posBase);
        int nextPosView  = incLane ? 1 : posViewNext;
        
        // Process
        int nextValue = getSignValue(data) + 1; 
        int next      = signs[nextValue];
        if(canMerge) addScore(nextValue); 

        // Memory
        int indexClear = computeIndex(lane, posView, dir);
        int indexWrite = computeIndex(lane, posBaseWrite, dir);
        int indexReadB = computeIndex(laneBaseRead, posBaseRead, dir);
        int indexReadD = computeIndex(nextLane, nextPosView, dir);;

        // printf("[%d:%d-%d] %x '%c' '%c' ", lane, posBase, posView, fieldIndex, buff, data);
        // printf("(%d%d%d%d%d%d%d) ", start, isViewZero, isBaseZero, canMerge, hasGap, hasTwoTiles,moveTile);
        // print_lane(lane, dir);
        // printf(" ---> ");

        if(hasTwoTiles || moveTile)
        {
            buff = canMerge ? next : data;

            if(moveTile)
            {
                accessMemory(indexClear, true, signs[0]);
                accessMemory(indexWrite, true, buff);

                if(canMerge) buff = signs[0];
            }
        }        

        if(start || incLane) buff = accessMemory(indexReadB, false, 0);
        if(!done)            data = accessMemory(indexReadD, false, 0);

        // printf("%x '%c' '%c' [%d:%d-%d] ", fieldIndex, buff, data, nextLane, nextPosBase, nextPosView);
        // print_lane(lane, dir);
        // printf("\n");

        start    = false;
        lane     = nextLane;
        posBase  = nextPosBase;
        posView  = nextPosView;
        hasMoved = hasMoved || moveTile;  


    } while(!done);

    // print_game();

    if(hasMoved) lastMove = dir;
    
    return hasMoved;
}


bool move2(move_direction_t dir)
{
    bool hasMoved = false;
    int accu = 'x', data = 'y';

    int lane = 0;
    int posAccu = 0;
    int posData = 0;
    bool done = false;
printf("===\n");
    do
    {       
        numSteps += 1;
        bool updateScore = false;

        bool start       = (posAccu == 0) && (posData == 0);
        bool isAccuZero  = (accu == signs[0]);
        bool isDataZero  = (data == signs[0]);    
        bool eqAccuData  = (accu == data);        
        bool hasTwoTiles = !start && !isAccuZero && !isDataZero;
        bool canMerge    = hasTwoTiles && eqAccuData;
        bool hasGap      = hasTwoTiles && (posData - posAccu > 1);
        bool moveTile    = !start && !isDataZero && (isAccuZero || canMerge || hasGap);

        bool incData     = canMerge;
        bool clear       = !start && !isDataZero && canMerge;
        bool load        = start || (!start && !isDataZero && !canMerge);

        int posAccuNext  = posAccu + 1;
        int posDataNext  = posData + 1;
        int laneNext     = lane + 1;
        int incLane      = (posDataNext >= NUM_BOARD);
        done             = incLane && (laneNext >= NUM_BOARD);

        int nextLane     = incLane ? laneNext : lane;
        int nextPosAccu  = incLane ? 0 : (!hasTwoTiles ? posAccu : (canMerge ? posData : posAccuNext));
        int nextPosData  = incLane ? 1 : posDataNext;
        
        int nextValue = getSignValue(data) + 1;  

        // printf("[%d:%d-%d] '%c' '%c' S:%d +:%d >:%d (%2d) D:%d --> %d:-%d-%d -- ", lane, posAccu, posData, accu, data, start, canMerge, moveTile, nextValue, done, nextLane, nextPosAccu, nextPosData);
  
        if(canMerge) addScore(nextValue); 

        // Memory
        int index1   = computeIndex(lane, posAccu, dir);
        int index1p1 = computeIndex(lane, posAccu+1, dir);
        int index2   = computeIndex(lane, posData, dir);
        int index1n  = computeIndex(nextLane, nextPosAccu, dir);
        int index2n  = computeIndex(nextLane, nextPosData, dir);

        int writeIndex = (!canMerge && hasGap) ? index1p1 : index1;
        
        // printf("%x-%x %x-%x %x\n", index1, index2, index1n, index2n, writeIndex);

        if(load) accu = incData ? signs[nextValue] : data;
        if(clear) accu = signs[0];

        int a = accu, d = data;

        if(moveTile) accessMemory(index2, true, signs[0]);
        if(moveTile) accessMemory(writeIndex, true, incData ? signs[nextValue] : data); //);
        if(start)    accu = accessMemory(index1n, false, 0);
        if(!done)    data = accessMemory(index2n, false, 0);

        printf("'%c' '%c' '%c' '%c'\n", a, d, accu, data);

        //printf("%d %d %d %d\n", index2, writeIndex, index1n, index2n);
        
        // if(load)  accu = data;
        //if(clear) accu = signs[0];
        // if(start) accu = data;

        lane     = nextLane;
        posAccu  = nextPosAccu;
        posData  = nextPosData;
        
        hasMoved = hasMoved || moveTile;  


    } while(!done);

    if(hasMoved) lastMove = dir;
    
    return hasMoved;
}

/*
Iterations: 5218
Steps: 640
*/
bool move3(move_direction_t dir)
{
    bool hasMoved = false;
    int accu = 0, data = 0;

    int lane = 0;
    int posAccu = 0;
    int posData = 0;
    bool done = false;

    do
    {       
        numSteps += 1;
        bool updateScore = false;


        int index1 = computeIndex(lane, posAccu, dir);
        int index1p1 = computeIndex(lane, posAccu+1, dir);
        int index2 = computeIndex(lane, posData, dir);
            
        data = accessMemory(index2, false, 0);
  
        bool start       = (posAccu == 0) && (posData == 0);
        bool isAccuZero  = (accu == signs[0]);
        bool isDataZero  = (data == signs[0]);    
        bool eqAccuData  = (accu == data);        
        bool hasTwoTiles = !start && !isAccuZero && !isDataZero;
        bool canMerge    = hasTwoTiles && eqAccuData;
        bool hasGap      = hasTwoTiles && (posData - posAccu > 1);
        bool moveTile    = !start && !isDataZero && (isAccuZero || canMerge || hasGap);

        bool incData     = canMerge;
        bool clear       = !start && !isDataZero && canMerge;
        bool load        = start || (!start && !isDataZero && !canMerge);

        int posAccuNext  = posAccu + 1;
        int posDataNext  = posData + 1;
        int laneNext     = lane + 1;
        int incLane      = (posDataNext >= NUM_BOARD);
        done             = incLane && (laneNext >= NUM_BOARD);

        int nextLane     = incLane ? laneNext : lane;
        int nextPosAccu  = incLane ? 0 : (!hasTwoTiles ? posAccu : (canMerge ? posData : posAccuNext));
        int nextPosData  = incLane ? 0 : posDataNext;
        
        if(!isDataZero)
        {
            if(isAccuZero)
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

        int nextValue = getSignValue(accu) + 1;    

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
        posAccu  = nextPosAccu;
        posData  = nextPosData;
        
        if(load)  accu = data;
        if(clear) accu = signs[0];

        hasMoved = hasMoved || moveTile;   

    } while(!done);

    if(hasMoved) lastMove = dir;
    
    return hasMoved;
}


bool canmove()
{
    return true;
}


void debug_computeIndex()
{
    for(int dir = 1; dir <= 4; dir++)
    {
        printf("direction: %s (%d)\n\n", moves[dir], dir);

        for(int lane = 0; lane < NUM_BOARD; lane++)
        {
            for(int pos = 0; pos < NUM_BOARD; pos++)
            {
                printf("%d.%d (%2d)\t", lane, pos, computeIndex(lane, pos, dir));
                // printf("assert(computeIndex(%d, %d, %d) == %d);\n", lane, pos, dir, computeIndex(lane, pos, dir));
            } 
            printf("\n");
        }

        printf("\n");
    }
}

void test_computeIndex()
{

    // direction: ↑ (1)

    assert(computeIndex(0, 0, MV_UP) == 12);
    assert(computeIndex(0, 1, MV_UP) == 8);
    assert(computeIndex(0, 2, MV_UP) == 4);
    assert(computeIndex(0, 3, MV_UP) == 0);

    assert(computeIndex(1, 0, MV_UP) == 13);
    assert(computeIndex(1, 1, MV_UP) == 9);
    assert(computeIndex(1, 2, MV_UP) == 5);
    assert(computeIndex(1, 3, MV_UP) == 1);

    assert(computeIndex(2, 0, MV_UP) == 14);
    assert(computeIndex(2, 1, MV_UP) == 10);
    assert(computeIndex(2, 2, MV_UP) == 6);
    assert(computeIndex(2, 3, MV_UP) == 2);

    assert(computeIndex(3, 0, MV_UP) == 15);
    assert(computeIndex(3, 1, MV_UP) == 11);
    assert(computeIndex(3, 2, MV_UP) == 7);
    assert(computeIndex(3, 3, MV_UP) == 3);


    // direction: ↓ (2)

    assert(computeIndex(0, 0, MV_DOWN) == 0);
    assert(computeIndex(0, 1, MV_DOWN) == 4);
    assert(computeIndex(0, 2, MV_DOWN) == 8);
    assert(computeIndex(0, 3, MV_DOWN) == 12);

    assert(computeIndex(1, 0, MV_DOWN) == 1);
    assert(computeIndex(1, 1, MV_DOWN) == 5);
    assert(computeIndex(1, 2, MV_DOWN) == 9);
    assert(computeIndex(1, 3, MV_DOWN) == 13);

    assert(computeIndex(2, 0, MV_DOWN) == 2);
    assert(computeIndex(2, 1, MV_DOWN) == 6);
    assert(computeIndex(2, 2, MV_DOWN) == 10);
    assert(computeIndex(2, 3, MV_DOWN) == 14);

    assert(computeIndex(3, 0, MV_DOWN) == 3);
    assert(computeIndex(3, 1, MV_DOWN) == 7);
    assert(computeIndex(3, 2, MV_DOWN) == 11);
    assert(computeIndex(3, 3, MV_DOWN) == 15);
    

    // direction: ← (3)

    assert(computeIndex(0, 0, MV_LEFT) == 3);
    assert(computeIndex(0, 1, MV_LEFT) == 2);
    assert(computeIndex(0, 2, MV_LEFT) == 1);
    assert(computeIndex(0, 3, MV_LEFT) == 0);

    assert(computeIndex(1, 0, MV_LEFT) == 7);
    assert(computeIndex(1, 1, MV_LEFT) == 6);
    assert(computeIndex(1, 2, MV_LEFT) == 5);
    assert(computeIndex(1, 3, MV_LEFT) == 4);

    assert(computeIndex(2, 0, MV_LEFT) == 11);
    assert(computeIndex(2, 1, MV_LEFT) == 10);
    assert(computeIndex(2, 2, MV_LEFT) == 9);
    assert(computeIndex(2, 3, MV_LEFT) == 8);

    assert(computeIndex(3, 0, MV_LEFT) == 15);
    assert(computeIndex(3, 1, MV_LEFT) == 14);
    assert(computeIndex(3, 2, MV_LEFT) == 13);
    assert(computeIndex(3, 3, MV_LEFT) == 12);


    // direction: → (4)

    assert(computeIndex(0, 0, MV_RIGHT) == 0);
    assert(computeIndex(0, 1, MV_RIGHT) == 1);
    assert(computeIndex(0, 2, MV_RIGHT) == 2);
    assert(computeIndex(0, 3, MV_RIGHT) == 3);

    assert(computeIndex(1, 0, MV_RIGHT) == 4);
    assert(computeIndex(1, 1, MV_RIGHT) == 5);
    assert(computeIndex(1, 2, MV_RIGHT) == 6);
    assert(computeIndex(1, 3, MV_RIGHT) == 7);

    assert(computeIndex(2, 0, MV_RIGHT) == 8);
    assert(computeIndex(2, 1, MV_RIGHT) == 9);
    assert(computeIndex(2, 2, MV_RIGHT) == 10);
    assert(computeIndex(2, 3, MV_RIGHT) == 11);

    assert(computeIndex(3, 0, MV_RIGHT) == 12);
    assert(computeIndex(3, 1, MV_RIGHT) == 13);
    assert(computeIndex(3, 2, MV_RIGHT) == 14);
    assert(computeIndex(3, 3, MV_RIGHT) == 15);
    
}

void test_computeIndex2()
{

    strcpy(field, "123456789abcdefg");

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
}

void debug_move()
{

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

    };

    char moveNames[5][9] = 
    {
        "",
        "MV_UP   ", 
        "MV_DOWN ", 
        "MV_LEFT ", 
        "MV_RIGHT"
    };

    int interationsTotal = 0;
    int stepsTotal = 0;

    int numTests = sizeof(test_fields)/sizeof(test_fields[0]);
    for(int i = 0; i < numTests; i++)
    {
       
        // printf("=== %d ===\n", i);
        strcpy(field, test_fields[i].test);
        // printf("\n"); print_game();
        fieldIndex = 0;
        numIterations = 0;
        numSteps = 0;
        bool moved = move(test_fields[i].dir);
        interationsTotal += numIterations;
        stepsTotal += numSteps;
        // printf("\n"); print_game();
        printf("'%s' %s '%s' (%s): '%s' (%s) [%3d][%2d]\n", test_fields[i].test, moves[test_fields[i].dir], test_fields[i].result, test_fields[i].moved ? "true " : "false", field, moved ? "true " : "false", numIterations, numSteps);
        
        assert(test_fields[i].moved == moved);
        assert(strcmp(field, test_fields[i].result) == 0);

        // printf("strcpy(field, \"%s\"); assert(move(%s) == %s); assert(strcmp(field, \"%s\") == 0);\n", test_fields[i].test, moveNames[test_fields[i].dir], test_fields[i].moved ? "true " : "false", test_fields[i].result);
    }  

    printf("\nIterations: %d\n", interationsTotal);
    printf("Steps: %d\n", stepsTotal);
}

void test_move()
{
    strcpy(field, "                "); assert(move(MV_DOWN ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_UP   ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_RIGHT) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_LEFT ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_DOWN ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_UP   ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_RIGHT) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_LEFT ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "     1          "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, " 1              ") == 0);
    strcpy(field, "     1          "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "             1  ") == 0);
    strcpy(field, "     1          "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "    1           ") == 0);
    strcpy(field, "     1          "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "       1        ") == 0);
    strcpy(field, "     1   1      "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, " 2              ") == 0);
    strcpy(field, "     1   1      "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "             2  ") == 0);
    strcpy(field, " 11             "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "2               ") == 0);
    strcpy(field, " 11             "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "   2            ") == 0);
    strcpy(field, "     1       2  "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, " 1   2          ") == 0);
    strcpy(field, " 1       2      "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "         1   2  ") == 0);
    strcpy(field, "     1 2        "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "    12          ") == 0);
    strcpy(field, "    1 2         "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "      12        ") == 0);
    strcpy(field, "     2       2  "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, " 3              ") == 0);
    strcpy(field, " 2       2      "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "             3  ") == 0);
    strcpy(field, "     2 2        "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "    3           ") == 0);
    strcpy(field, "    2 2         "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "       3        ") == 0);
    strcpy(field, "  1           2 "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "  1   2         ") == 0);
    strcpy(field, "  1           2 "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "          1   2 ") == 0);
    strcpy(field, "    2  2        "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "    3           ") == 0);
    strcpy(field, "    2  2        "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "       3        ") == 0);
    strcpy(field, "  2           2 "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "  3             ") == 0);
    strcpy(field, "  2           2 "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "              3 ") == 0);
    strcpy(field, "    2  2        "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "    3           ") == 0);
    strcpy(field, "    2  2        "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "       3        ") == 0);
    strcpy(field, "3   3   1   1   "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "4   2           ") == 0);
    strcpy(field, "3   3   1   1   "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "        4   2   ") == 0);
    strcpy(field, "3311            "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "42              ") == 0);
    strcpy(field, "3311            "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "  42            ") == 0);
    strcpy(field, "1   1   1   1   "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "2   2           ") == 0);
    strcpy(field, "1   1   1   1   "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "        2   2   ") == 0);
    strcpy(field, "1111            "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "22              ") == 0);
    strcpy(field, "1111            "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "  22            ") == 0);

}

int main()
{
    int ch;
    bool moved = true;
    size_t numMoves = 0;

    // debug_move();
    // debug_spawn_tetrisrng();
    test_computeIndex();
    test_computeIndex2();
    test_move();
    // return 0;
    GETCH();

    strcpy(field, "                ");
    strcpy(score, "      0");
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

            printf("\nfield: '%s' score: %s last spawn: %d last move: %s step: %zu\n", field, score, lastSpawn, moves[lastMove], numMoves);
            print_game();
            
            moved = false;
        }


        if (!canmove())
        {
            printf("\nGame Over!");
            break;
        }

        printf("\rmove?> ");
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
