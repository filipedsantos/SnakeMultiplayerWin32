#define _WIN32_WINNT 0x0500
#include <Windows.h>
#include <sddl.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include "SnakeServer.h"

void(*setInfoSHM)();

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

HANDLE clients[MAXCLIENTS];
HANDLE hMapFile;

HANDLE eWriteToClientNP;

HANDLE eReadFromClientSHM;
HANDLE eWriteToClientSHM;

HANDLE hThreadSharedMemory;

HINSTANCE hSnakeDll;

GameInfo gameInfo;
Game game;

int IDcounter = MINIDPLAYER;
int gameCount;

HANDLE hGameThread;

//MAIN 
int _tmain(void){

	#ifdef UNICODE
		 _setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif
	
	_tprintf(TEXT("\nSTARTING SERVER.........................OK\n\n"));
	initializeServer();
	
	WaitForSingleObject(hThreadSharedMemory, INFINITE);

	return 0;
}	


///////////////////////
// Functions --------//
///////////////////////


void initializeServer() {

	int(*ptr)();

	//LOADING SnakeDll
	hSnakeDll = LoadLibraryEx(TEXT("..\\..\\SnakeMultiplayerWin32_dll\\Debug\\SnakeMultiplayerWin32_dll.dll"), NULL, 0);
	if (hSnakeDll == NULL) {
		_tprintf(TEXT("[ERROR] Dll not available... (%d)\n"), GetLastError());
		return;
	}

	ptr = (int(*)()) GetProcAddress(hSnakeDll, "snakeFunction");
	if (ptr == NULL) {
		_tprintf(TEXT(">>[SHM ERROR] GetProcAddress\n"));
		return;
	}

	//CHECK DLL FUNCTION
	if(ptr()==111)
		_tprintf(TEXT("DLL correctly Loaded >> ...............%d\n"), ptr());
	else {
		_tprintf(TEXT("DLL not correctly Loaded >> ...............%d\n"), ptr());
	}

	initializeSharedMemory();
	initializeNamedPipes();

	FreeLibrary(hSnakeDll);
}


//PREPARE SHAREDMEMORY THREAD
//
//creates a mapfile and struct through DLL methods for comunication 
//between client & server and launch thread related to sh memory

void initializeSharedMemory() {

	pCircularBuff circularBufferPointer = NULL;

	BOOL(*createFileMap)();
	pCircularBuff(*getCircularBufferPointerSHM)();


	_tprintf(TEXT("STARTING SHARED MEMORY....................\n"));

	//CREATEFILEMAP
	createFileMap = (BOOL(*)()) GetProcAddress(hSnakeDll, "createFileMapping");
	if (createFileMap == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading createFileMapping function from DLL (%d)\n"), GetLastError());
		return;
	}

	if (!createFileMap()) {
		_tprintf(TEXT("[SHM ERROR] Creating File Map Object... (%d)"), GetLastError());
		return;
	}

	setInfoSHM = (void(*)()) GetProcAddress(hSnakeDll, "setInfoSHM");
	if (setInfoSHM == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading getDataSHM function from DLL (%d)\n"), GetLastError());
		return;
	}

	//CREATE A THREAD RESPONSABLE FOR SHM ONLY
	hThreadSharedMemory = CreateThread(
		NULL,
		0,
		listenClientSharedMemory,
		NULL,
		0,
		0);

	if (hThreadSharedMemory == NULL) {
		_tprintf(TEXT("[ERROR] Creating Shared Memory Thread... (%d)"), GetLastError());
		return;
	}
	_tprintf(TEXT("Shared Memory has started correctly.......\n"));
}


//CREATE A THREAD FOR EACH USER "LOGGED IN"
//
//mains a cycle waiting for clients, then it creates a 
//individual thread for each user connected to namedpipe

void initializeNamedPipes() {

	BOOL fconnected = FALSE;
	DWORD dwThreadID = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	LPTSTR lpszPipeName = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");



	eWriteToClientNP = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (eWriteToClientNP == NULL) {
		_tprintf(TEXT("\n Error creating event write - %d\n"), GetLastError());
		return -1;
	}


	//////MAIN CYCLE OF THE PROGRAM

	startClients();
	_tprintf(TEXT("STARTING NAMEDPIPES.......................\n"));

	//////CREATE NAMEDPIPE THEN CREATE THREAD FOR USER

	SECURITY_ATTRIBUTES sa;
	TCHAR *szSD = TEXT("(A;OICI;GA;;;AU)");
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;

	//ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &(sa->lpSecurityDescriptor), NULL);

	while (1) {
		hPipe = CreateNamedPipe(
			lpszPipeName,			 //name
			PIPE_ACCESS_DUPLEX |
			FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,				 //mode message-read, blocked
			PIPE_UNLIMITED_INSTANCES,//255 instances
			BUFFSIZE,
			BUFFSIZE,
			5000,					 //TIMEOUT
			NULL);	//&sa					 //DEFAULT SECURITY	


		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("NAMEDPIPED FAILED.... ERROR: %d\n"), GetLastError());
			return -1;
		}

		fconnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		//IF THE CONNECTION HAS BEEN DONE
		if (fconnected) {
			_tprintf(TEXT("New Client Connected...\n"));

			hThread = CreateThread(
				NULL,
				0,
				listenClientNamedPipes,
				(LPVOID)hPipe, //PARAM PASSED
				0,
				&dwThreadID);  //ID


			if (hThread == NULL) {
				_tprintf(TEXT("\n Error creating a new thread : %d"), GetLastError());
				return -1;
			}
			else
				CloseHandle(hThread); 

		}
		else {

			_tprintf(TEXT("\nClient didn't connect"));
			CloseHandle(hPipe);
		}
	}
}


////////////////////////////
//NAMEDPIPE FUCTIONS	////
////////////////////////////


//CREATE A MESSAGE TO CLIENT
void writeClients(HANDLE client, GameInfo gi) {
	DWORD cbWritten = 0;
	BOOL  success = FALSE;

	OVERLAPPED overLapped = { 0 };

	ZeroMemory(&overLapped, sizeof(overLapped));
	ResetEvent(eWriteToClientNP);
	overLapped.hEvent = eWriteToClientNP;

	success = WriteFile(
		client,
		&gi,
		GameInfoStructSize,
		&cbWritten,
		&overLapped);

	WaitForSingleObject(eWriteToClientNP, INFINITE);

	GetOverlappedResult(client, &overLapped, &cbWritten, FALSE);
	if (cbWritten < GameInfoStructSize)
		_tprintf(TEXT("\nIncomplete data... Error: %d"), GetLastError());

}

//SEND TO EVERYCLIENT THE SAME MESSAGE
void broadcastClients(GameInfo gi) {
	int i = 0;
	for (i; i < MAXCLIENTS; i++) {
		if (clients[i] != 0) {
			writeClients(clients[i], gi);
		}
	}
}

//CREATE A NEW CLIENT
void addClient(HANDLE hPipe) {
	int i = 0;
	for (i; i<MAXCLIENTS; i++) {

		if (clients[i] == NULL) {

			clients[i] = hPipe;
			return;
		}
	}
}

//REMOVE CLIENT
void removeClients(HANDLE hPipe) {

	int i = 0;
	for (i; i < MAXCLIENTS; i++) {

		if (clients[i] == hPipe) {
			clients[i] = NULL;
			return;
		}
	}
}

//START ALL CLIENTS WITH NULL
void startClients() {
	int i = 0;

	for (i; i<MAXCLIENTS; i++) {
		clients[i] = NULL;
	}

}


////////////////////////
//GAME  Functions////
////////////////////////

//START BOARD GAME INITIALIZED EVERYTHING WITH '0'
Snake initSnake(int size, int rows, int columns, int id) {
	Snake snake;
	int startX, startY;
	snake.coords = malloc(sizeof(Coords) * size);

	srand(time(NULL));
	int orientation = rand() % 2;

	do {
		srand(time(NULL));
		startX = rand() % (columns - 1) + 1;
		srand(time(NULL));
		startY = rand() % (rows - 1) + 1;
	} while (!verifyPosition(size, startX, startY, orientation));

	if (orientation == 0) {	// Horizontal
		for (int i = 0; i < size; i++) {
			snake.coords[i].posX = startX + i;
			snake.coords[i].posY = startY;
		}
	}
	else {			// Vertical
		for (int i = 0; i < size; i++) {
			snake.coords[i].posX = startX;
			snake.coords[i].posY = startY + i;
		}
	}

	snake.direction = rand() % 4 + 1;
	snake.alive = TRUE;
	snake.size = size;

	snake.id = id;
	snake.print = id;
	snake.speed = NORMAL_SPEED;
	snake.effect = NO_EFFECT;
	

	putSnakeIntoBoard(-1, -1, snake);
	return snake;
}

BOOL verifyPosition(int size, int rows, int columns, int orientation) {
	if (orientation == 0) {		// Horizontal
		for (int i = 0; i < size; i++) {
			if (game.boardGame[rows][columns + i] != 0) {
				return FALSE;
			}
		}
	}
	else {						// Vertical
		for (int i = 0; i < size; i++) {
			if (game.boardGame[rows + i][columns] != 0) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

void joinGame(data dataGame) {

	// Initialize the Player and the Snake
	_tcscpy(game.playerSnakes[game.nPlayers].nickname, dataGame.nicknamePlayer1);
	game.playerSnakes[game.nPlayers] = initSnake(game.snakeInitialSize, game.nRows, game.nColumns, dataGame.playerId);
	game.nPlayers++;

	if (dataGame.numLocalPlayers == 2) {
		_tcscpy(game.playerSnakes[game.nPlayers].nickname, dataGame.nicknamePlayer2);
		game.playerSnakes[game.nPlayers] = initSnake(game.snakeInitialSize, game.nRows, game.nColumns, dataGame.playerId2);
		game.nPlayers++;
	}

	gameInfo.commandId = JOIN_GAME;		// Put the id to send gameInfo
	gameInfo.playerId = dataGame.playerId;
	sendInfoToPlayers(gameInfo);

	
}

void initGame(data dataGame) {

	game.nRows = dataGame.nRows;
	game.nColumns = dataGame.nColumns;
	game.Created = TRUE;
	game.running = FALSE;
	game.snakeInitialSize = dataGame.serpentInitialSize;
	game.nPlayers = 0;
	game.objectsDuration = dataGame.objectsDuration;
	game.nObjects = dataGame.gameObjects;


	// Initialize board
	game.boardGame = malloc(sizeof(int) * game.nRows);
	for (int i = 0; i < game.nRows; i++)	{
		game.boardGame[i] = malloc(sizeof(int)* game.nColumns);
	}

	for (int i = 0; i < game.nRows; i++) {
		for (int j = 0; j < game.nColumns; j++) {
			game.boardGame[i][j] = BLOCK_EMPTY;
			if (i == 0 || j == 0 || i == game.nRows - 1 || j == game.nColumns - 1) {
				game.boardGame[i][j] = BLOCK_WALL;
			}
		}
	}

	// Initialize the Player and the Snake
	_tcscpy(game.playerSnakes[0].nickname, dataGame.nicknamePlayer1);
	game.playerSnakes[0] = initSnake(dataGame.serpentInitialSize, dataGame.nRows, dataGame.nColumns, dataGame.playerId);
	game.nPlayers++;

	if (dataGame.numLocalPlayers == 2) {
		_tcscpy(game.playerSnakes[1].nickname, dataGame.nicknamePlayer2);
		game.playerSnakes[1] = initSnake(dataGame.serpentInitialSize, dataGame.nRows, dataGame.nColumns, dataGame.playerId2);
		game.nPlayers++;
	}

	// Initialize some objets
	game.object = malloc(sizeof(Objects) * dataGame.gameObjects);

	for (int i = 0; i < 9; i++) {
		game.objectPercentages[i] = dataGame.objects[i];
	}

	initObjetcts();
	

	_tprintf(TEXT("\n\n"));
	for (int i = 0; i < game.nRows; i++) {
		for (int j = 0; j < game.nRows; j++) {
			_tprintf(TEXT(" %d "), game.boardGame[i][j]);
		}
		_tprintf(TEXT("\n"));
	}
	_tprintf(TEXT("end objects\n"));

	

	gameInfo.commandId = START_GAME;
}

void initObjetcts() {
	
	for (int i = 0; i < game.nObjects; i++)
	{
		game.object[i] = initRandomObject();
	}
}

Objects initRandomObject() {
	int x, y;
	Objects randomizedObject;

	int perc = 0;
	int nGenerated;
	int percFood = game.objectPercentages[0];
	int percIce = game.objectPercentages[1];
	int percGranade = game.objectPercentages[2];
	int percVodka = game.objectPercentages[3];
	int percOil = game.objectPercentages[4];
	int percGlue = game.objectPercentages[5];
	int percOvodka = game.objectPercentages[6];
	int percOoil = game.objectPercentages[7];
	int percOglue = game.objectPercentages[8];

	do {
		x = rand() % game.nColumns;
		y = rand() % game.nRows;
	} while (game.boardGame[x][y] != 0);

	randomizedObject.x = x;
	randomizedObject.y = y;
	randomizedObject.duration = game.objectsDuration;

	nGenerated = rand() % 100 + 1;
	perc = 0;

	if (nGenerated < (perc = perc + game.objectPercentages[0])) {				// Food
		game.boardGame[x][y] = BLOCK_FOOD;
		randomizedObject.block = BLOCK_FOOD;
		randomizedObject.duration = 0;

	}
	else if (nGenerated < (perc = perc + game.objectPercentages[1])) {			// Ice
		game.boardGame[x][y] = BLOCK_ICE;
		randomizedObject.block = BLOCK_ICE;
		randomizedObject.duration = 0;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[2])) {			// Granade
		game.boardGame[x][y] = BLOCK_GRANADE;
		randomizedObject.block = BLOCK_GRANADE;
		randomizedObject.duration = 0;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[3])) {			// Vodka
		game.boardGame[x][y] = BLOCK_VODKA;
		randomizedObject.block = BLOCK_VODKA;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[4])) {			// Oil
		game.boardGame[x][y] = BLOCK_OIL;
		randomizedObject.block = BLOCK_OIL;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[5])) {			// Glue
		game.boardGame[x][y] = BLOCK_GLUE;
		randomizedObject.block = BLOCK_GLUE;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[6])) {			// O-Vodka
		game.boardGame[x][y] = BLOCK_O_VODKA;
		randomizedObject.block = BLOCK_O_VODKA;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[7])) {			// O-Oil
		game.boardGame[x][y] = BLOCK_O_OIL;
		randomizedObject.block = BLOCK_O_OIL;
	}
	else if (nGenerated < (perc = perc + game.objectPercentages[8])) {			// O-Glue
		game.boardGame[x][y] = BLOCK_O_GLUE;
		randomizedObject.block = BLOCK_O_GLUE;
	}
	else {
		game.boardGame[x][y] = BLOCK_EMPTY;
	}

	return randomizedObject;
}

Snake verifyEffect(Snake snake) {

	if (snake.effect != NO_EFFECT) {
		snake.timeEffect--;
		if (snake.timeEffect == 0) {
			snake.effect = NO_EFFECT;
			snake.speed  = NORMAL_SPEED;
		}
	}

	return snake;
}

void oEffect(int block) {
	for (size_t i = 0; i < game.nPlayers; i++){
		switch (block)
		{
		case BLOCK_O_GLUE:
			game.playerSnakes[i].speed = SLOW_SPEED;
			game.playerSnakes[i].effect = EFFECT_SPEED;
			game.playerSnakes[i].timeEffect = game.objectsDuration;
			break;
		case BLOCK_O_OIL:
			game.playerSnakes[i].speed = RACE_SPEED;
			game.playerSnakes[i].effect = EFFECT_SPEED;
			game.playerSnakes[i].timeEffect = game.objectsDuration;
			break;
		case BLOCK_O_VODKA:
			game.playerSnakes[i].effect = EFFECT_DRUNK;
			game.playerSnakes[i].timeEffect = game.objectsDuration;
			break;
		default:
			break;
		}
	}
	

}

void putSnakeIntoBoard(int delX, int delY, Snake snake) {

	
	for (int i = 0; i < snake.size; i++) {
		game.boardGame[snake.coords[i].posY][snake.coords[i].posX] = snake.print;
	}
	if (delX >= 0 && delY >= 0) {
		game.boardGame[delY][delX] = 0;
	}
	
}

Snake move(Snake snake) {
	Coords *toEat;
	Coords toMove = snake.coords[0];
	int delX=-1, delY=-1;

	snake = verifyEffect(snake);

	switch (snake.direction) {
		case RIGHT:
			toMove.posX += 1;
			toMove.posY = snake.coords[0].posY;
			break;
		case LEFT:
			toMove.posX -= 1;
			toMove.posY = snake.coords[0].posY;
			break;
		case UP:
			toMove.posX = snake.coords[0].posX;
			toMove.posY -= 1;
			break;
		case DOWN:
			toMove.posX = snake.coords[0].posX;
			toMove.posY += 1;
			break;
	}

	// Get position to check
	int positionToCheck = game.boardGame[toMove.posY][toMove.posX];
	
	// Verify the position to go before move the snake
	switch (positionToCheck) {

		case BLOCK_EMPTY:
			break;
		case BLOCK_WALL:
			// kill the snake
			snake.alive = FALSE;
			snake.print = 0;
			delX = delY = -1;
			break;

		case BLOCK_FOOD:
			// Grow the snake
			snake.size++;
			toEat = malloc(sizeof(Coords) * snake.size);
			toEat[0] = toMove;
			for (int i = 1; i < snake.size; i++) {
				toEat[i] = snake.coords[i - 1];
			}
			snake.coords = toEat;
			break;
		case BLOCK_ICE:
			for (int i = 0; i < snake.size; i++){
				game.boardGame[snake.coords[i].posY][snake.coords[i].posX] = 0;
			}

			snake.size--;
			toEat = malloc(sizeof(Coords) * snake.size);
			toEat[0] = toMove;
			for (int i = 1; i < snake.size; i++) {
				toEat[i] = snake.coords[i - 1];
			}
			snake.coords = toEat;
			break;
		case BLOCK_GRANADE:
			delX = snake.coords[snake.size - 1].posX;
			delY = snake.coords[snake.size - 1].posY;
			toEat = malloc(sizeof(Coords) * snake.size);
			toEat[0] = toMove;
			for (int i = 1; i < snake.size; i++) {
				toEat[i] = snake.coords[i - 1];
			}
			snake.coords = toEat;
			
			// kill the snake
			snake.alive = FALSE;
			snake.print = 0;
	
			break;
		case BLOCK_VODKA:
			snake.effect = EFFECT_DRUNK;
			snake.timeEffect = game.objectsDuration;
			break;
		case BLOCK_OIL:
			snake.speed = RACE_SPEED;
			snake.effect = EFFECT_SPEED;
			snake.timeEffect = game.objectsDuration;
			break;
		case BLOCK_GLUE:
			snake.speed = SLOW_SPEED;
			snake.effect = EFFECT_SPEED;
			snake.timeEffect = game.objectsDuration;
			break;
		case BLOCK_O_VODKA:
			oEffect(BLOCK_O_VODKA);
			break;
		case BLOCK_O_OIL:
			oEffect(BLOCK_O_OIL);
			break;
		case BLOCK_O_GLUE:
			oEffect(BLOCK_O_GLUE);
			break;
		default:
			break;
	}

	//COLISION WITH ANOTHER SNAKE AND SET SCORE +10 TO ANOTHER SNAKE

	if (positionToCheck > 1000) {
		// kill the snake
		snake.alive = FALSE;
		snake.print = 0;
		delX = delY = -1;

		for (int i = 0; i < game.nPlayers; i++){
			if (positionToCheck == game.playerSnakes[i].id) {

				game.playerSnakes[i].score += 10;

			}
		}
	}

	if (positionToCheck == snake.id) {
		// kill the snake
		snake.alive = FALSE;
		snake.print = 0;
		delX = delY = -1;
	}

	if (snake.alive && positionToCheck != BLOCK_FOOD && positionToCheck != BLOCK_GRANADE ){
		//&& positionToCheck != BLOCK_ICE && positionToCheck != BLOCK_GRANADE && positionToCheck != BLOCK_VODKA && positionToCheck != BLOCK_OIL && positionToCheck != BLOCK_GLUE && positionToCheck != BLOCK_O_VODKA && positionToCheck != BLOCK_O_OIL && positionToCheck != BLOCK_O_GLUE) {

		// Get position to delete in case of move
		delX = snake.coords[snake.size - 1].posX;
		delY = snake.coords[snake.size - 1].posY;

		// Copy the coordinates of the snake to apply movement 
		for (int i = snake.size - 1; i > 0; i--) {
			snake.coords[i].posX = snake.coords[i - 1].posX;
			snake.coords[i].posY = snake.coords[i - 1].posY;
		}

		// Apply the movement to the head
		switch (snake.direction) {
			case RIGHT:
				snake.coords[0].posX += 1;
				snake.direction = RIGHT;
				break;
			case LEFT:
				snake.coords[0].posX -= 1;
				snake.direction = LEFT;
				break;
			case UP:
				snake.coords[0].posY -= 1;
				snake.direction = UP;
				break;
			case DOWN:
				snake.coords[0].posY += 1;
				snake.direction = DOWN;
				break;
		}
	}
	
	Sleep(snake.speed);
	// Draw the snake on the map
	putSnakeIntoBoard(delX, delY, snake);

	return snake;
}

void updateGameInfoBoard() {
	gameInfo.commandId = REFRESH_BOARD;
	gameInfo.playerId = 1000;	// For all players see
	gameInfo.nRows = game.nRows;
	gameInfo.nColumns = game.nColumns;

	for (int i = 0; i < game.nRows; i++) {
		for (int j = 0; j < game.nColumns; j++) {
			gameInfo.boardGame[i][j] = game.boardGame[i][j];
		}
	}

}

BOOL verifyEndGame() {

	for (int i = 0; i < MAXCLIENTS; i++)
	{
		if (game.playerSnakes[i].alive) {
			return;
		}
	}
	game.Created = FALSE;
	game.running = FALSE;

}

void sendInfoToPlayers(GameInfo gi) {
	// Write on SHM
	setInfoSHM(gi);
	SetEvent(eWriteToClientSHM);

	// Broadcast to NamedPipes
	broadcastClients(gi);
	SetEvent(eWriteToClientNP);

	resetGameInfo();

}

void resetGameInfo() {
	gameInfo.playerId = -1;														
	gameInfo.commandId = -1;
	// falta limpar o reston nao sei se e necesario
}

void moveSnakes() {
	for (int i = 0; i < MAXCLIENTS; i++) {
		if (game.playerSnakes[i].alive) {
			game.playerSnakes[i] = move(game.playerSnakes[i]);
		}
	}

}

void manageCommandsReceived(data dataGame) {
	switch (dataGame.op) {
	case EXIT:
		_tprintf(TEXT("Goodbye.."));

		break;
	case CREATE_GAME:
		if (!game.Created) {
			initGame(dataGame);
		}
		else {
			gameInfo.commandId = ERROR_CANNOT_CREATE_GAME;
			gameInfo.playerId = dataGame.playerId;
			sendInfoToPlayers(gameInfo);
		}

		break;
	case START_GAME:
		if (!game.running) {
			hGameThread = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)gameThread,
				NULL,
				0,
				0
			);
			if (hGameThread == NULL) {
				_tprintf(TEXT("[ERROR] Impossible to create gameThread... (%d)"), GetLastError());
				return -1;
			}
			game.running = TRUE;
		}

		break;
	case JOIN_GAME:
		if (game.Created) {
			joinGame(dataGame);
		}
		else {
			gameInfo.commandId = ERROR_CANNOT_JOIN_GAME;
			gameInfo.playerId = dataGame.playerId;
			sendInfoToPlayers(gameInfo);
		}
		break;
	case SCORES:
		break;
	case MOVE_SNAKE:
		moveIndividualSnake(dataGame.playerId, dataGame.direction);
		break;
	case MOVE_SNAKE2:
		moveIndividualSnake(dataGame.playerId2, dataGame.direction);
		break;
	default:
		break;
	}
}

void moveIndividualSnake(int id, int direction) {
	for (int i = 0; i < MAXCLIENTS; i++) {
		if (game.playerSnakes[i].id == id && game.playerSnakes[i].alive) {

			if (game.playerSnakes[i].effect == EFFECT_DRUNK) {
				switch (direction)
				{
				case RIGHT:
					direction = LEFT;
					break;
				case LEFT:
					direction = RIGHT;
					break;
				case UP:
					direction = DOWN;
					break;
				case DOWN:
					direction = UP;
					break;
				default:
					break;
				}

			}


			switch (direction) {
			case RIGHT:
				if (game.playerSnakes[i].direction != LEFT && game.playerSnakes[i].direction != RIGHT) {
					game.playerSnakes[i].direction = RIGHT;
				}
				break;
			case LEFT:
				if (game.playerSnakes[i].direction != RIGHT  && game.playerSnakes[i].direction != LEFT) {
					game.playerSnakes[i].direction = LEFT;
				}
				break;
			case UP:
				if (game.playerSnakes[i].direction != DOWN  && game.playerSnakes[i].direction != DOWN) {
					game.playerSnakes[i].direction = UP;
				}
				break;
			case DOWN:
				if (game.playerSnakes[i].direction != UP  && game.playerSnakes[i].direction != UP) {
					game.playerSnakes[i].direction = DOWN;
				}
				break;
			default:
				break;
			}
		}
		//diretionToGo = 0;
	}

}


//////////////
// THREADS ///
//////////////

//---------------------------------------------------------
// THREAD RESPONSABLE FOR ATTENDING CLIENTS VIA NAMEDPIPED
//---------------------------------------------------------

DWORD WINAPI listenClientNamedPipes (LPVOID param){
	data dataGame;
	BOOL success = FALSE;
	DWORD cbBytesRead  = 0;
	DWORD cbBytesReply = 0;

	HANDLE hPipe = (HANDLE) param; //param received is a handle of the pipe
	OVERLAPPED overLapped = { 0 };

	HANDLE ReadFromClient;

	//TEST PIPE 
	if(hPipe == NULL){
		_tprintf(TEXT("\n[THREAD]: Handle is null %d"), GetLastError());
		return -1;
	}

	ReadFromClient = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(ReadFromClient == NULL){
		_tprintf(TEXT("\nError creating Read event - %d"), GetLastError());
		return -1;
	}
	addClient(hPipe);

	while (1) {

		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(ReadFromClient);
		overLapped.hEvent = ReadFromClient;

		//READ FROM CLIENT
		success = ReadFile(
			hPipe,				//READ CHANNEL
			&dataGame,				//BUFFER OF READING DATA
			DataStructSize,    //SIZE OF STRUCT
			&cbBytesRead,
			&overLapped);

		WaitForSingleObject(ReadFromClient, INFINITE);

		GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE);
		if (cbBytesRead < DataStructSize) {
			_tprintf(TEXT("\n[THREAD] ReadFile does not read all - %d"), GetLastError());
		}

		///////////////////////////////////////

	/*	_tprintf(TEXT("[%s] : [%s]\n"), request.who, request.command);

		_tcscpy(reply.command, request.command);
		broadcastClients(reply);*/

		manageCommandsReceived(dataGame);
	}


	removeClients(hPipe);

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);


	_tprintf(TEXT("\n [NAMEDPIPE THREAD] terminated...."));
	return 0;
}

//---------------------------------------------------
// THREAD RESPONSABLE FOR HANDLING SHAREDMEMORY
//---------------------------------------------------

DWORD WINAPI listenClientSharedMemory(LPVOID params) {
	data dataGame;
	eReadFromClientSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM"));
	eWriteToClientSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM_eWriteToClientSHM"));

	while (1) {
		data(*getDataSHM)();

		//Wait for any client trigger the event by typing any option
		WaitForSingleObject(eReadFromClientSHM, INFINITE);
		
		
		//GETDATA IN CORRECT PULL POSITION
		getDataSHM = (pData(*)()) GetProcAddress(hSnakeDll, "getDataSHM");
		if (getDataSHM == NULL) {
			_tprintf(TEXT("[SHM ERROR] Loading getDataSHM function from DLL (%d)\n"), GetLastError());
			return;
		}
		dataGame = getDataSHM();
		manageCommandsReceived(dataGame);
		ResetEvent(eReadFromClientSHM);
	}	
}

//---------------------------------------------------
// GAMING THREAD
//---------------------------------------------------

DWORD WINAPI gameThread(LPVOID params) {
	_tprintf(TEXT("\n-----GAMETHREAD----\n"));

	gameCount = 0;

	while (game.running) {
		gameCount++;
		moveSnakes();

		/*_tprintf(TEXT("\n\n"));
		for (int i = 0; i < game.nRows; i++) {
			for (int j = 0; j < game.nRows; j++) {
				_tprintf(TEXT(" %d "), game.boardGame[i][j]);
			}
			_tprintf(TEXT("\n"));
		}*/

		updateGameInfoBoard();
		sendInfoToPlayers(gameInfo);
		
		verifyEndGame();

		Sleep(450);

	}

	// send info to termiante thread
	gameInfo.commandId = GAME_OVER;
	sendInfoToPlayers(gameInfo);

	_tprintf(TEXT("\nThread terminou..\n"));
}


