#pragma once

#define	BUFFSIZE	1024
#define	TCHARSIZE	30

#define	WHO			60
#define	COMMANDSIZE 60

#define	SIZECIRCULARBUFFER 20

#define	LOCALCLIENT		0
#define	REMOTECLIENT	1

// STRUCTS

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

int typeClient = -1;

void askTypeClient();
void startLocalClient();
void startRemoteClient();
void createGame();
void gameMenu();


DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

int mayContinue = 1;
int readerAlive = 0;

