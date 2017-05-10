#pragma once
#include <tchar.h>

// Defines
#define BUFFSIZE 1024

#define WHO 		60
#define COMMANDSIZE 60
#define SIZECIRCULARBUFFER 20

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

typedef struct data {
	TCHAR who[WHO];
	TCHAR command[COMMANDSIZE];
	int op;							// Option 
	int nPlayers;					// Number of players to join the created game
	int nLines;
	int nColumns;
} data, *pData;


typedef struct sCircularBuffer {
	data circularBuffer[SIZECIRCULARBUFFER];
	int pull;
	int push;
} sCircularBuffer, *pCircularBuff;

#define BuffsizeCircularBuff sizeof(sCircularBuffer)

_declspec(dllexport) int snakeFunction();

_declspec(dllexport) pCircularBuff createFileMapping();

_declspec(dllexport) pCircularBuff openFileMapping();

_declspec(dllexport) pCircularBuff getSHM(HANDLE hMapFile, BOOL createNewStruct);

pCircularBuff createNewCircularBuffer(pCircularBuff cb);

_declspec(dllexport) void setDataSHM(pData data);

_declspec(dllexport) void getDataSHM(pData data);