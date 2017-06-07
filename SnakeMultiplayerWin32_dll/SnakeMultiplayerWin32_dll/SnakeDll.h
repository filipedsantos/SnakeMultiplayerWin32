#pragma once
#include <tchar.h>

// Defines
#define BUFFSIZE 1024
#define TCHARSIZE 30

#define WHO 		60
#define COMMANDSIZE 60
#define SIZECIRCULARBUFFER 20

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

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

// VARIABLES

// FUNCTIONS TO EXPORT

_declspec(dllexport) int snakeFunction();

_declspec(dllexport) BOOL createFileMapping();

_declspec(dllexport) BOOL openFileMapping();

//_declspec(dllexport) pCircularBuff getCircularBufferPointerSHM();
//
//_declspec(dllexport) pGameInfo getGameInfoPointerSHM();

_declspec(dllexport) void setDataSHM(data data);

_declspec(dllexport) data getDataSHM();

_declspec(dllexport) void setInfoSHM(GameInfo gi);

_declspec(dllexport) GameInfo getInfoSHM();

_declspec(dllexport) HANDLE startSyncSemaphore(BOOL writer);

_declspec(dllexport) HANDLE startSyncMutex();

// DLL FUNCTIONS

void releaseSyncHandles(HANDLE mClient, HANDLE semaphoreWrite);

void createNewCircularBuffer();