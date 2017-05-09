#pragma once
#include <tchar.h>

// Defines
#define BUFFSIZE 1024

#define WHOS		60
#define COMMANDSIZE 60

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");


typedef struct data {
	TCHAR who[WHOS];
	TCHAR command[COMMANDSIZE];
	int op;			// Option 
	int nPlayers;	// Number of players to join the created game
	int nLines;
	int nColumns;
} data, *pData;

_declspec(dllexport) int snakeFunction();

_declspec(dllexport) HANDLE createFileMapping();

_declspec(dllexport) HANDLE openFileMapping();

_declspec(dllexport) pData getSHM(HANDLE hMapFile);