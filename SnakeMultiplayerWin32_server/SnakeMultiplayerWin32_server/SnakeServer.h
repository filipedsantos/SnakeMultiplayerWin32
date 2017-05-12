#pragma once
#include "Player.h"
#include "Snake.h"

// Defines
#define BUFFSIZE 1024
#define TCHARSIZE 30

#define WHO 		60
#define COMMANDSIZE 60
#define MAXCLIENTS  20

#define EXIT 0
#define CREATE_GAME 1
#define JOIN_GAME 2
#define SCORES 3

#define SIZECIRCULARBUFFER 20

// STRUCTS

typedef struct data {
	TCHAR who[WHO];
	TCHAR command[COMMANDSIZE];
	int op;			// Option 
	int nPlayers;	// Number of players to join the created game
	int nLines;
	int nColumns;
} data, *pData;

typedef struct Game {
	int ** boardGame;

	int nPlayers;
	Player players;
	
	//int nSnakes;	// Contador para saber se ha cobras ainda a jogar !
	Snake snakes;
} Game, *pGame;

typedef struct Scores {
	TCHAR playerName[TCHARSIZE];
	int score;
}Scores, *pScores;

// Struct to send info about the actual state of game to the client
typedef struct GameInfo {

	//TCHAR message[BUFFSIZE];			// variable to send some additional info to client
	int commandId;						// variable to inform client about the actual command
	Scores scores[SIZECIRCULARBUFFER];	// array to send info about scores
	int ** boardGame;					// variable that send information about the game variables - snakes, food, etc...	
} GameInfo, *pGameInfo;

#define GameStructSize sizeof(GameInfo)

typedef struct sCircularBuffer {
	data circularBuffer[SIZECIRCULARBUFFER];
	int pull;
	int push;
} sCircularBuffer, *pCircularBuff;

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