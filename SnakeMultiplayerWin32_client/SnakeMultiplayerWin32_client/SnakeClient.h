#pragma once

#define BUFFSIZE 1024
#define WHO 60
#define COMMANDSIZE 60

#define SIZECIRCULARBUFFER 20

#define LOCALCLIENT 0
#define REMOTECLIENT 1

typedef struct data {
	TCHAR who[WHO];
	TCHAR command[COMMANDSIZE];
	int op;			// Option 
	int nPlayers;	// Number of players to join the created game
	int nLines;
	int nColumns;
} data, *pData;

typedef struct sCircularBuffer {
	data circularBuffer[SIZECIRCULARBUFFER];
	int pull;
	int push;
} sCircularBuffer, *pCircularBuff;

#define dataSize sizeof(data)

int typeClient = -1;

void askTypeClient();
void startLocalClient();
void startRemoteClient();
void createGame();
void gameMenu();

DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

int mayContinue = 1;
int readerAlive = 0;

