#include <tchar.h>

#define MAXNAMESIZE 15

typedef struct Player {
	int id;
	TCHAR name[MAXNAMESIZE];
	
} Player, *pPlayer;