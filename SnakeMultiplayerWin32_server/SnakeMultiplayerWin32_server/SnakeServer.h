#pragma once
#include "Player.h"
#include "Snake.h"

// Defines
#define BUFFSIZE 1024

#define WHOS		60
#define COMMANDSIZE 60
#define MAXCLIENTS  20

#define EXIT 0
#define CREATE_GAME 1
#define JOIN_GAME 2
#define SCORES 3


typedef struct data {
	TCHAR who[WHOS];
	TCHAR command[COMMANDSIZE];
	int op;			// Option 
	int nPlayers;	// Number of players to join the created game
	int nLines;
	int nColumns;
} data, *pData;

typedef struct Game {
	int gameBoard[10][10];

	int nPlayers;
	Player players;
	
	//int nSnakes;	// Contador para saber se ha cobras ainda a jogar !
	Snake snakes;
} Game, *pGame;


#define dataSize sizeof(data)



// Functions
void startClients();
void addClient(HANDLE hPipe);
void removeClients(HANDLE hPipe);
void writeClients(HANDLE client, data dataReply);
void broadcastClients(data dataReply);
void initializeServer();
void initializeNamedPipes();
void initializeSharedMemory();

// Threads
DWORD WINAPI listenClientNamedPipes(LPVOID params);
DWORD WINAPI listenClientSharedMemory(LPVOID params);

DWORD WINAPI gameThread(LPVOID params);