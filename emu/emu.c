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


#define NUM_FIELD_WIDTH 4
#define NUM_FIELD (NUM_FIELD_WIDTH * NUM_FIELD_WIDTH)
#define NUM_SCORE 7
#define NUM_SIGNS 18

static move_direction_t lastMove = 0;
static unsigned int lastSpawn = 0;
static unsigned char moves[][4] = { " ", "↑", "↓", "←", "→" };
static unsigned char field[NUM_FIELD + 1] = "                ";
static unsigned int fieldIndex = 0;
static unsigned int numIterations = 0;
static unsigned char score[NUM_SCORE + 1] = "      0";
static unsigned char signs[NUM_SIGNS] = {' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

void print_game()
{
    /*
    
    ╔═══╦═══╦═══╦═══╗
    ║   ║   ║   ║   ║
    ╟───╫───╫───╫───╢
    ║   ║   ║   ║   ║
    ╟───╫───╫───╫───╢
    ║   ║   ║   ║   ║
    ╟───╫───╫───╫───╢
    ║   ║   ║   ║   ║
    ╚═══╩═══╩═══╩═══╝
    
    */

   printf("╔═══╦═══╦═══╦═══╗\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[0], field[1], field[2], field[3]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[4], field[5], field[6], field[7]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[8], field[9], field[10], field[11]);
   printf("╟───╫───╫───╫───╢\n");
   printf("║ %c ║ %c ║ %c ║ %c ║\n", field[12], field[13], field[14], field[15]);
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

              0         0 1 2 3
              1         4 5 6 7
              2         8 9 a b
              3         c d e f
         */
        case MV_LEFT: return (lane * NUM_FIELD_WIDTH) + pos;

        /* 
            lane    pos 3 2 1 0

              0         0 1 2 3
              1         4 5 6 7
              2         8 9 a b
              3         c d e f
         */
        case MV_RIGHT: return (lane * NUM_FIELD_WIDTH) + (NUM_FIELD_WIDTH - pos - 1);

        /* 
             pos   lane 0 1 2 3

              0         0 1 2 3
              1         4 5 6 7
              2         8 9 a b
              3         c d e f
         */
        case MV_UP: return lane + (pos * NUM_FIELD_WIDTH);

        /* 
             pos  lane  0 1 2 3

              3         0 1 2 3
              2         4 5 6 7
              1         8 9 a b
              0         c d e f
         */
        case MV_DOWN: return lane + ((NUM_FIELD_WIDTH - pos - 1) * NUM_FIELD_WIDTH);

    }

    assert(false);
    return -1;
}



int accessMemory(int index, bool write, int data)
{
    if(index >= fieldIndex) numIterations += fieldIndex - index;
    else                    numIterations += (NUM_FIELD - fieldIndex) + index;

    fieldIndex = index;

    if(write) 
    {
        field[index] = data;
    }
    
    return field[index];
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

bool move(move_direction_t dir)
{
    bool hasMoved = false;
    int data1, data2;

    for(int lane = 0; lane < NUM_FIELD_WIDTH; lane++)
    {
        int pos1 = 0;
        int pos2 = 1;
    
        bool fetch1 = true;
        bool done = false;

        do
        {       

            bool writeData1 = false;
            bool writeData1NextIndex;
            bool clearData2 = false;
            bool updateScore = false;

            int nextPos1 = pos1;
            int nextPos2 = pos2 + 1;

            int index1 = computeIndex(lane, pos1, dir);
            int index2 = computeIndex(lane, pos2, dir);
            int index1p1 = computeIndex(lane, pos1+1, dir);
            
            if(fetch1) data1 = accessMemory(index1, false, 0);
            data2 = accessMemory(index2, false, 0);

            fetch1 = false;

            int nextData1 = data1;
            int writeData1Value;

            bool pos1empty = (data1 == signs[0]);
            bool pos2empty = (data2 == signs[0]);            
            bool canMerge  = (data1 == data2);
            int nextValue  = getSignValue(data1) + 1;
            bool hasGap = (pos2 - pos1 > 1);


            if(!pos2empty)
            {
                if(pos1empty)
                {
                    /*
                        move
                        1   2       1     2 
                        . . 1 1 --> 1 . . 1 
                    */

                    writeData1 = true;
                    writeData1Value = data2;
                    nextData1 = data2;
                    clearData2 = true;
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
                        writeData1 = true;
                        writeData1Value = signs[nextValue];

                        nextPos1  = pos2;
                        nextData1 = signs[0];

                        updateScore = true;
                        clearData2  = true;

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

                            index1 = index1p1;

                            writeData1 = true;
                            writeData1Value = data2;                        

                            nextData1  = data2;

                            clearData2 = true;
                        }
                        else
                        {
                            /*
                                check next value
                                1 2           1 2
                                1 2 . 2 --> 1 2 . 2
                            */
                            nextData1 = data2;
                            nextPos2 = pos1 + 2; 
                        }

                        nextPos1 = pos1 + 1;
                    }
                }
            }   

            // printf("\n%d:%d-%d empty:%d:%d gap:%d canMerge:%d", lane, pos1, pos2, pos1empty, pos2empty, hasGap, canMerge);  
            // if(writeData1) printf(" [%d]<-%c", pos1, writeData1Value);
            // if(clearData2) printf(" [%d]<-0", pos2);

            if(writeData1) accessMemory(index1, true, writeData1Value);
            if(clearData2) accessMemory(index2, true, signs[0]);
            if(updateScore) addScore(nextValue);

            pos1 = nextPos1;
            pos2 = nextPos2;

            data1 = nextData1;

            if(nextPos2 >= NUM_FIELD_WIDTH) done = true;

            hasMoved = hasMoved || writeData1;   

        } while(!done);

    }

    // printf("\n");
    // print_game();

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

        for(int lane = 0; lane < NUM_FIELD_WIDTH; lane++)
        {
            for(int pos = 0; pos < NUM_FIELD_WIDTH; pos++)
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

    assert(computeIndex(0, 0, 1) == 0);
    assert(computeIndex(0, 1, 1) == 4);
    assert(computeIndex(0, 2, 1) == 8);
    assert(computeIndex(0, 3, 1) == 12);

    assert(computeIndex(1, 0, 1) == 1);
    assert(computeIndex(1, 1, 1) == 5);
    assert(computeIndex(1, 2, 1) == 9);
    assert(computeIndex(1, 3, 1) == 13);

    assert(computeIndex(2, 0, 1) == 2);
    assert(computeIndex(2, 1, 1) == 6);
    assert(computeIndex(2, 2, 1) == 10);
    assert(computeIndex(2, 3, 1) == 14);

    assert(computeIndex(3, 0, 1) == 3);
    assert(computeIndex(3, 1, 1) == 7);
    assert(computeIndex(3, 2, 1) == 11);
    assert(computeIndex(3, 3, 1) == 15);


    // direction: ↓ (2)

    assert(computeIndex(0, 0, 2) == 12);
    assert(computeIndex(0, 1, 2) == 8);
    assert(computeIndex(0, 2, 2) == 4);
    assert(computeIndex(0, 3, 2) == 0);

    assert(computeIndex(1, 0, 2) == 13);
    assert(computeIndex(1, 1, 2) == 9);
    assert(computeIndex(1, 2, 2) == 5);
    assert(computeIndex(1, 3, 2) == 1);

    assert(computeIndex(2, 0, 2) == 14);
    assert(computeIndex(2, 1, 2) == 10);
    assert(computeIndex(2, 2, 2) == 6);
    assert(computeIndex(2, 3, 2) == 2);

    assert(computeIndex(3, 0, 2) == 15);
    assert(computeIndex(3, 1, 2) == 11);
    assert(computeIndex(3, 2, 2) == 7);
    assert(computeIndex(3, 3, 2) == 3);


    // direction: ← (3)

    assert(computeIndex(0, 0, 3) == 0);
    assert(computeIndex(0, 1, 3) == 1);
    assert(computeIndex(0, 2, 3) == 2);
    assert(computeIndex(0, 3, 3) == 3);

    assert(computeIndex(1, 0, 3) == 4);
    assert(computeIndex(1, 1, 3) == 5);
    assert(computeIndex(1, 2, 3) == 6);
    assert(computeIndex(1, 3, 3) == 7);

    assert(computeIndex(2, 0, 3) == 8);
    assert(computeIndex(2, 1, 3) == 9);
    assert(computeIndex(2, 2, 3) == 10);
    assert(computeIndex(2, 3, 3) == 11);

    assert(computeIndex(3, 0, 3) == 12);
    assert(computeIndex(3, 1, 3) == 13);
    assert(computeIndex(3, 2, 3) == 14);
    assert(computeIndex(3, 3, 3) == 15);


    // direction: → (4)

    assert(computeIndex(0, 0, 4) == 3);
    assert(computeIndex(0, 1, 4) == 2);
    assert(computeIndex(0, 2, 4) == 1);
    assert(computeIndex(0, 3, 4) == 0);

    assert(computeIndex(1, 0, 4) == 7);
    assert(computeIndex(1, 1, 4) == 6);
    assert(computeIndex(1, 2, 4) == 5);
    assert(computeIndex(1, 3, 4) == 4);

    assert(computeIndex(2, 0, 4) == 11);
    assert(computeIndex(2, 1, 4) == 10);
    assert(computeIndex(2, 2, 4) == 9);
    assert(computeIndex(2, 3, 4) == 8);

    assert(computeIndex(3, 0, 4) == 15);
    assert(computeIndex(3, 1, 4) == 14);
    assert(computeIndex(3, 2, 4) == 13);
    assert(computeIndex(3, 3, 4) == 12);
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
        {"                ", MV_UP,    "                ", false},
        {"                ", MV_DOWN,  "                ", false},
        {"                ", MV_LEFT,  "                ", false},
        {"                ", MV_RIGHT, "                ", false},
        {"123456789abcdefg", MV_UP,    "123456789abcdefg", false},
        {"123456789abcdefg", MV_DOWN,  "123456789abcdefg", false},
        {"123456789abcdefg", MV_LEFT,  "123456789abcdefg", false},
        {"123456789abcdefg", MV_RIGHT, "123456789abcdefg", false},
        
        // move
        {"     1          ", MV_UP,    " 1              ", true},
        {"     1          ", MV_DOWN,  "             1  ", true},
        {"     1          ", MV_LEFT,  "    1           ", true},
        {"     1          ", MV_RIGHT, "       1        ", true},

        // merge
        {"     1   1      ", MV_UP,    " 2              ", true},
        {"     1   1      ", MV_DOWN,  "             2  ", true},
        {" 11             ", MV_LEFT,  "2               ", true},
        {" 11             ", MV_RIGHT, "   2            ", true},

        // move with gap
        {"     1       2  ", MV_UP,    " 1   2          ", true},
        {" 1       2      ", MV_DOWN,  "         1   2  ", true},
        {"     1 2        ", MV_LEFT,  "    12          ", true},
        {"    1 2         ", MV_RIGHT, "      12        ", true},

        // merge with gap
        {"     2       2  ", MV_UP,    " 3              ", true},
        {" 2       2      ", MV_DOWN,  "             3  ", true},
        {"     2 2        ", MV_LEFT,  "    3           ", true},
        {"    2 2         ", MV_RIGHT, "       3        ", true},

        // combined test
        {"3   3   1   1   ", MV_UP,    "4   2           ", true},
        {"3   3   1   1   ", MV_DOWN,  "        4   2   ", true},
        {"3311            ", MV_LEFT,  "42              ", true},
        {"3311            ", MV_RIGHT, "  42            ", true},

        // multi merge
        {"1   1   1   1   ", MV_UP,    "2   2           ", true},
        {"1   1   1   1   ", MV_DOWN,  "        2   2   ", true},
        {"1111            ", MV_LEFT,  "22              ", true},
        {"1111            ", MV_RIGHT, "  22            ", true}

    };

    char moveNames[5][9] = 
    {
        "",
        "MV_UP   ", 
        "MV_DOWN ", 
        "MV_LEFT ", 
        "MV_RIGHT"
    };

    int numTests = sizeof(test_fields)/sizeof(test_fields[0]);
    for(int i = 0; i < numTests; i++)
    {
        strcpy(field, test_fields[i].test);
        if(i == (numTests - 1)) { printf("\n"); print_game(); }
        bool moved = move(test_fields[i].dir);
        if(i == (numTests - 1)) { printf("\n"); print_game(); }
        printf("'%s' %s '%s' (%s): '%s' (%s)\n", test_fields[i].test, moves[test_fields[i].dir], test_fields[i].result, test_fields[i].moved ? "true " : "false", field, moved ? "true " : "false");

        // printf("strcpy(field, \"%s\"); assert(move(%s) == %s); assert(strcmp(field, \"%s\") == 0);\n", test_fields[i].test, moveNames[test_fields[i].dir], test_fields[i].moved ? "true " : "false", test_fields[i].result);
    }  


}

void test_move()
{
    strcpy(field, "                "); assert(move(MV_UP   ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_DOWN ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_LEFT ) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "                "); assert(move(MV_RIGHT) == false); assert(strcmp(field, "                ") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_UP   ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_DOWN ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_LEFT ) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "123456789abcdefg"); assert(move(MV_RIGHT) == false); assert(strcmp(field, "123456789abcdefg") == 0);
    strcpy(field, "     1          "); assert(move(MV_UP   ) == true ); assert(strcmp(field, " 1              ") == 0);
    strcpy(field, "     1          "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "             1  ") == 0);
    strcpy(field, "     1          "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "    1           ") == 0);
    strcpy(field, "     1          "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "       1        ") == 0);
    strcpy(field, "     1   1      "); assert(move(MV_UP   ) == true ); assert(strcmp(field, " 2              ") == 0);
    strcpy(field, "     1   1      "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "             2  ") == 0);
    strcpy(field, " 11             "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "2               ") == 0);
    strcpy(field, " 11             "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "   2            ") == 0);
    strcpy(field, "     1       2  "); assert(move(MV_UP   ) == true ); assert(strcmp(field, " 1   2          ") == 0);
    strcpy(field, " 1       2      "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "         1   2  ") == 0);
    strcpy(field, "     1 2        "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "    12          ") == 0);
    strcpy(field, "    1 2         "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "      12        ") == 0);
    strcpy(field, "     2       2  "); assert(move(MV_UP   ) == true ); assert(strcmp(field, " 3              ") == 0);
    strcpy(field, " 2       2      "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "             3  ") == 0);
    strcpy(field, "     2 2        "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "    3           ") == 0);
    strcpy(field, "    2 2         "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "       3        ") == 0);
    strcpy(field, "3   3   1   1   "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "4   2           ") == 0);
    strcpy(field, "3   3   1   1   "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "        4   2   ") == 0);
    strcpy(field, "3311            "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "42              ") == 0);
    strcpy(field, "3311            "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "  42            ") == 0);
    strcpy(field, "1   1   1   1   "); assert(move(MV_UP   ) == true ); assert(strcmp(field, "2   2           ") == 0);
    strcpy(field, "1   1   1   1   "); assert(move(MV_DOWN ) == true ); assert(strcmp(field, "        2   2   ") == 0);
    strcpy(field, "1111            "); assert(move(MV_LEFT ) == true ); assert(strcmp(field, "22              ") == 0);
    strcpy(field, "1111            "); assert(move(MV_RIGHT) == true ); assert(strcmp(field, "  22            ") == 0);


}

int main()
{
    int ch;
    bool moved = true;
    size_t numMoves = 0;

    test_computeIndex();
    test_move();
    debug_spawn_tetrisrng();

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
