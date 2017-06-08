#pragma once

#define	BUFFSIZE	1024
#define	TCHARSIZE	30

#define	WHO			60
#define	COMMANDSIZE 60

#define	SIZECIRCULARBUFFER 20

#define	LOCALCLIENT		0
#define	REMOTECLIENT	1

#define EXIT			100
#define CREATE_GAME		101
#define JOIN_GAME		102
#define SCORES			103
#define START_GAME		104
#define MOVE_SNAKE		105

#define LEFT  1
#define RIGHT 2
#define UP    3
#define DOWN  4

// STRUCTS
typedef struct data {
	TCHAR who[WHO];
	TCHAR command[COMMANDSIZE];
	int op;			// Option 
	int nPlayers;	// Number of players to join the created game
	int nRows;
	int nColumns;
	int direction;
} data, *pData;

#define DataStructSize sizeof(data)

typedef struct sCircularBuffer {
	data circularBuffer[SIZECIRCULARBUFFER];
	int pull;
	int push;
} sCircularBuffer, *pCircularBuff;

#define BuffsizeCircularBuff sizeof(sCircularBuffer)

typedef struct Scores {
	TCHAR playerName[TCHARSIZE];
	int score;
}Scores, *pScores;

// Struct to send info about the actual state of game to the client
typedef struct GameInfo {

	//TCHAR message[BUFFSIZE];			// variable to send some additional info to client
	int commandId;						// variable to inform client about the actual command
	Scores scores[SIZECIRCULARBUFFER];	// array to send info about scores
	int boardGame[10][10];					// variable that send information about the game variables - snakes, food, etc...
	int nRows, nColumns;
} GameInfo, *pGameInfo;

#define GameInfoStructSize sizeof(GameInfo)
// END STRUCTS

// Variables
BOOL created;
BOOL runningThread = FALSE;
HANDLE hMovementThread;
int move;

// Threads

//DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

DWORD WINAPI ThreadClientReaderSHM(LPVOID PARAMS);
DWORD WINAPI movementThread(LPVOID lpParam);

int mayContinue = 1;
int readerAlive = 0;

