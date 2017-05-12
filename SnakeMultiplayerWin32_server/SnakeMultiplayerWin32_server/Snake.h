#include <tchar.h>

#include "Coords.h"

#define LEFT 1
#define TOP 2
#define RIGHT 3
#define BOT 4

typedef struct Snake {
	int id;
	Coords * coords;
	int size;

	boolean alive;
	int direction;

} Snake, *pSnake;