// gcc -no-pie -Os -s -o snake snake.c -lasound -lm
#define LORES_H_IMPLEMENTATION
#include "lores.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define ARENA_W 16
#define ARENA_H 16
#define ARENA_SZ (ARENA_W*ARENA_H)

#define EMPTY 0
#define HEAD 1
#define WALL -1
#define FOOD -2

enum {
	STATE_INIT,
	STATE_RUNNING,
	STATE_PAUSE,
	STATE_OVER,
	STATE_WON,
};

int arena[ARENA_SZ];
int arena_index(int x, int y) { return y*ARENA_W+x; }
int arena_get(int x, int y) { return arena[y*ARENA_W+x]; }
void arena_set(int x, int y, int value) { arena[y*ARENA_W+x] = value; }
int arena_neighbor(int idx, int dir) {
	return dir==0 ? idx-ARENA_W : dir==1 ? idx+1 : dir==2 ? idx+ARENA_W : idx-1;
}
int arena_randomEmptyCell() {
	int count = 0;
	for (int i = 0; i < ARENA_SZ; ++i)
		if (arena[i] == EMPTY) ++count;
	if (!count) return -1;
	int choice = randi(count);
	for (int i = 0; i < ARENA_SZ; ++i)
		if (arena[i] == EMPTY && choice-- == 0) return i;
	return -1;
}

uint64_t frame=0;
int state=STATE_INIT;
uint32_t highscore = 0;
char* info = NULL;

typedef struct { int x, y, dir, length, head, tail; } Snake;

void Snake_init(Snake* snake) {
	snake->x = ARENA_W/2;
	snake->y = ARENA_H/2;
	snake->dir = randi(4);
	snake->length = 1;
	snake->head = snake->tail = arena_index(snake->x, snake->y);
	arena[snake->head] = HEAD;
}

bool Snake_moveTo(Snake* snake, int nb, bool grow) {
	arena[nb] = HEAD;
	arena[snake->head] = nb;
	snake->head = nb;
	if(grow) {
		sound(880,50);
		++snake->length;
		const int empty = arena_randomEmptyCell();
		if (empty >= 0)
			arena[empty] = FOOD;
		else {
			state = STATE_WON;
			info = "Y O U  W I N!";
			if ((uint32_t)(snake->length-1) > highscore)
				highscore = snake->length-1;
			frame = 0;
		}
	}
	else {
		int tail = snake->tail;
		snake->tail = arena[snake->tail];
		arena[tail] = EMPTY;
	}
	return true;
} 

bool Snake_step(Snake* snake) {
	const int nbIdx = arena_neighbor(snake->head, snake->dir), nb = arena[nbIdx];
	switch(nb) {
	case EMPTY:
		return Snake_moveTo(snake, nbIdx, false);
	case FOOD:
		return Snake_moveTo(snake, nbIdx, true);
	}
	return false;
}

Snake snake;

//------------------------------------------------------------------
void AppInit() {
	for(int y=0; y<ARENA_H; ++y) for(int x=0; x<ARENA_W; ++x)
		arena_set(x,y, !y||!x||(x+1==ARENA_W)||(y+1==ARENA_H) ? WALL : EMPTY);
	Snake_init(&snake);
	arena[arena_randomEmptyCell()] = FOOD;
	frame = 0;
	state = STATE_RUNNING;
	info = NULL;
}

void AppUpdate() {
	const int framesPerStep = 1;
	if(state==STATE_RUNNING && ++frame%framesPerStep == 0 && !Snake_step(&snake)) {
		state = STATE_OVER;
		sound(180,250);
		if(snake.length-1>highscore) {
			highscore = snake.length-1;
		}
		frame = 0;
	}
	else if((state==STATE_OVER || state==STATE_WON) && ++frame>10)
		AppInit();
}

void AppDraw() {
	const uint16_t TILE_SZ = 4;
	static BlockBuf* bb = NULL;
	static uint8_t *tWall, *tFood, *tHead, *tBody;
	if(!bb) {
		bb = BlockBufCreate(ARENA_W*TILE_SZ, ARENA_H*TILE_SZ);
		const char colorsWall[] = {3,' ',COLOR_LIGHT_GRAY,'*', COLOR_WHITE, '#', COLOR_GRAY};
		tWall = BlockBufDecode("*** " "*  #" "*  #" " ###", TILE_SZ, TILE_SZ, colorsWall, 0);
		static const char tileSnake[] = " ** " "*o.*" "*..*" " ** ";
		const char colorsHead[] = {3,'*',COLOR_YELLOW, '.',COLOR_LIGHT_YELLOW, 'o', COLOR_WHITE};
		tHead = BlockBufDecode(tileSnake, TILE_SZ, TILE_SZ, colorsHead, 0);
		const char colorsBody[] = {3,'*',COLOR_RED, '.',COLOR_LIGHT_RED, 'o', COLOR_LIGHT_RED};
		tBody = BlockBufDecode(tileSnake, TILE_SZ, TILE_SZ, colorsBody, 0);
		const char colorsFood[] = {3,'*',COLOR_GREEN, '.',COLOR_LIGHT_GREEN, 'o', COLOR_LIGHT_YELLOW};
		tFood = BlockBufDecode(tileSnake, TILE_SZ, TILE_SZ, colorsFood, 0);
	}
	for(int y=0; y<ARENA_H; ++y) {
		for(int x=0; x<ARENA_W; ++x) {
			switch(arena_get(x,y)) {
			case EMPTY: BlockBufFill(bb,x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ, COLOR_BLACK); break;
			case WALL: BlockBufBlt(bb, x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ, tWall); break;
			case FOOD: BlockBufBlt(bb,x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ, tFood); break;
			case HEAD: BlockBufBlt(bb,x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ, tHead); break;
			default: BlockBufBlt(bb,x*TILE_SZ, y*TILE_SZ, TILE_SZ, TILE_SZ, tBody); break;
			}
		}
	}
	BlockBufOut(bb,1,1);
	//gotoxy(1,ARENA_H*TILE_SZ + 1);
	textcolor(COLOR_LIGHT_CYAN);
	printf("\nscore:%i best:%u", snake.length-1, highscore);
	clreol();
	if(info) {
		textcolor(COLOR_WHITE);
		printf("\n%s", info);
	}
	clreol();
}

//------------------------------------------------------------------
int main(int argc, char** argv) {
	initscr();
	atexit(shutdownscr);
	clrscr();
	echooff();
	cursoroff();
	colorReset();

	AppInit();

	int c=0;
	while(c!=KEY_ESCAPE) {
		if(kbhit()) {
			c = getkey();
			switch(c) {
			case 'p':
				if(state == STATE_RUNNING) {
					state = STATE_PAUSE;
					info = "P A U S E D.";
				}
				else if(state == STATE_PAUSE) {
					state = STATE_RUNNING;
					info = NULL;
				}
				break;
			case KEY_UP:    if(snake.dir!=2) snake.dir = 0; break;
			case KEY_DOWN:  if(snake.dir!=0) snake.dir = 2; break;
			case KEY_LEFT:  if(snake.dir!=1) snake.dir = 3; break;
			case KEY_RIGHT: if(snake.dir!=3) snake.dir = 1; break;
			}
		}
		AppUpdate();
		AppDraw();
		delay(180);
	}

	gotoxy(1,ARENA_H+2);
	clreol();
	shutdownscr();
	return 0;
}