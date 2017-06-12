#pragma once
#include <windows.h>

// Defines
#define MINIDPLAYER 1000
#define BUFFSIZE 1024
#define TCHARSIZE 30

#define TCHAR_SIZE 		60
#define WHO 		60
#define COMMANDSIZE 60
#define MAXCLIENTS  20

#define SIZECIRCULARBUFFER 20

#define EXIT			100
#define CREATE_GAME		101
#define JOIN_GAME		102
#define SCORES			103
#define START_GAME		104
#define MOVE_SNAKE		105
#define MOVE_SNAKE2		106




//errors
#define ERROR_CANNOT_CREATE_GAME 506

//messages
#define SHM_ALL 600


#define LEFT  1
#define RIGHT 2
#define UP    3
#define DOWN  4



// STRUCTS
typedef struct data {
	TCHAR who[WHO];
	TCHAR command[COMMANDSIZE];

	int op;					// Command ID 
	int numLocalPlayers;	// Number of players to join the created game
	int nRows;
	int nColumns;

	int typeOfGame;				// SinglePlayer or MultiPlayer
	int gameObjects;
	int objectsDuration;
	int serpentInitialSize;
	int AIserpents;

	int playerId;
	TCHAR nicknamePlayer1[TCHAR_SIZE];

	int playerId2;
	TCHAR nicknamePlayer2[TCHAR_SIZE];

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
	int playerId;						// id for wich player send info (1000 for all)
										//TCHAR message[BUFFSIZE];			// variable to send some additional info to client
	int commandId;						// variable to inform client about the actual command
	Scores scores[SIZECIRCULARBUFFER];	// array to send info about scores
	int boardGame[100][100];			// variable that send information about the game variables - snakes, food, etc...
	int nRows, nColumns;
} GameInfo, *pGameInfo;

// Struct to send info about the actual state of game to the client

#define GameInfoStructSize sizeof(GameInfo)

#define BLOCK_EMPTY			0
#define BLOCK_WALL			1
#define BLOCK_FOOD			2
#define BLOCK_ICE			3
#define BLOCK_GRANADE		4
#define BLOCK_VODKA			5
#define BLOCK_OIL			6
#define BLOCK_GLUE			7
#define BLOCK_O_VODKA		8
#define BLOCK_O_OIL			9
#define BLOCK_O_GLUE		11


typedef struct Coords {
	int posX;
	int posY;
} Coords, *pCoords;

typedef struct Snake {
	
	//PLAYER
	TCHAR nickname[TCHARSIZE];
	int id;
	int score;
	
	//SNAKE
	Coords *coords;
	int size;
	BOOL alive;
	int direction;
	int print;		// save the id of the snake/player to print ; when snake die this print = 0 and not change the original id

} Snake, *pSnake;

typedef struct Game {

	//int commandId;						
	Scores scores[SIZECIRCULARBUFFER];
	int  **boardGame;
	BOOL Created;
	BOOL running;
	int nPlayers;
	int nRows, nColumns;
	Snake playerSnakes[MAXCLIENTS];

} Game, *pGame;

// END STRUCTS

// Functions
void startClients();
void addClient(HANDLE hPipe);
void removeClients(HANDLE hPipe);
void writeClients(HANDLE client, GameInfo gi);
void broadcastClients(GameInfo gi);
void initializeServer();
void initializeNamedPipes();
void initializeSharedMemory();
void initGame(data dataGame);
void putSnakeIntoBoard(int delX, int delY, Snake snake);
Snake move(Snake snake);
Snake initSnake(int startX, int startY, int size, int id);
void updateGameInfo();
void initObjetcts(int objectType, int nObjects);
void moveIndividualSnake(int id, int direction);
void moveSnakes();
void verifyEndGame();
void sendInfoToPlayers(GameInfo gi);
void manageCommandsReceived(data dataGame);

// Threads
DWORD WINAPI listenClientNamedPipes(LPVOID params);
DWORD WINAPI listenClientSharedMemory(LPVOID params);
DWORD WINAPI gameThread(LPVOID params);