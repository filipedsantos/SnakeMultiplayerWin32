#pragma once

#define	BUFFSIZE	1024
#define	TCHARSIZE	30

#define TCHAR_SIZE	60
#define	WHO			60
#define	COMMANDSIZE 60

#define	SIZECIRCULARBUFFER 20

#define	LOCALCLIENT		0
#define	REMOTECLIENT	1

#define SINGLEPLAYER	0
#define MULTIPLAYER		1

#define EXIT			100
#define CREATE_GAME		101
#define JOIN_GAME		102
#define SCORES			103
#define START_GAME		104
#define MOVE_SNAKE		105
#define MOVE_SNAKE2		106
#define GAME_OVER		107
#define REFRESH_BOARD	108

#define ERROR_CANNOT_CREATE_GAME	99
#define ERROR_CANNOT_JOIN_GAME		98

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
	int objects[9];
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

#define GameInfoStructSize sizeof(GameInfo)
// END STRUCTS

// Variables
BOOL created;
BOOL runningThread = FALSE;
HANDLE hMovementThread;
int move = RIGHT;

// Functions

LRESULT CALLBACK MainWindow(HWND, UINT, WPARAM, LPARAM);
ATOM registerClass(HINSTANCE hInst, TCHAR * szWinName);
HWND CreateMainWindow(HINSTANCE hInst, TCHAR * szWinName);
BOOL CALLBACK DialogTypeUser(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogEditControls(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogAuthenticate(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);

void closeEverything();
void startMainWindow();
void startLocal();
void startRemote();
void sendCommand(data newData);
void createErrorMessageBox(TCHAR *message);
void createMessageBox(TCHAR *Message);
void bitmap(left, right, top, bot);
void editResourceOnPaint(int resource);
void manageCommandsReceived(GameInfo gameInfo);

// Threads

//DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

DWORD WINAPI ThreadClientReaderSHM(LPVOID PARAMS);
DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

int mayContinue = 1;
int readerAlive = 0;

