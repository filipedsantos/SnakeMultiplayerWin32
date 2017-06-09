#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include "SnakeServer.h"

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

HANDLE clients[MAXCLIENTS];
HANDLE hMapFile;

HANDLE eWriteToClientNP;

HANDLE eReadFromClientSHM;
HANDLE eWriteToClientSHM;

HANDLE hThreadSharedMemory;

HINSTANCE hSnakeDll;
int diretionToGo = 0;
int x, y;

data dataGame;
GameInfo gameInfo;

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
			NULL);					 //DEFAULT SECURITY	


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
void writeClients(HANDLE client, data dataReply) {
	DWORD cbWritten = 0;
	BOOL  success = FALSE;

	OVERLAPPED overLapped = { 0 };

	ZeroMemory(&overLapped, sizeof(overLapped));
	ResetEvent(eWriteToClientNP);
	overLapped.hEvent = eWriteToClientNP;

	success = WriteFile(
		client,
		&dataReply,
		DataStructSize,
		&cbWritten,
		&overLapped);

	WaitForSingleObject(eWriteToClientNP, INFINITE);

	GetOverlappedResult(client, &overLapped, &cbWritten, FALSE);
	if (cbWritten < DataStructSize)
		_tprintf(TEXT("\nIncomplete data... Error: %d"), GetLastError());

}

//SEND TO EVERYCLIENT THE SAME MESSAGE
void broadcastClients(data dataReply) {
	int i = 0;
	for (i; i < MAXCLIENTS; i++) {
		if (clients[i] != 0) {
			writeClients(clients[i], dataReply);
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
//THREADS  Functions////
////////////////////////

//START BOARD GAME INITIALIZED EVERYTHING WITH '0'
Snake initSnake(int startX, int startY, int size) {
	Snake snake;
	snake.coords = malloc(sizeof(Coords) * size);

	srand(time(NULL));
	int r = rand() % 2;

	if (r == 0) {	// Horizontal
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

	//snake.direction = rand() % 4 + 1;
	snake.direction = LEFT;
	snake.alive = TRUE;
	snake.size = size;

	// Falta adicionar um ID na criacao da snake (Player ID)
	snake.id = 5;
	snake.print = snake.id;
	

	return snake;
}

void initGameInfo() {
	x = y = 0;

	//change 3 for a variable from edit control

	for (int i = 0; i < dataGame.nRows; i++) {
		for (int j = 0; j < dataGame.nColumns; j++) {

			gameInfo.boardGame[i][j] = 0;

		}
	}

	gameInfo.nRows = dataGame.nRows;
	gameInfo.nColumns = dataGame.nColumns;
}

void putSnakeIntoBoard(int delX, int delY, Snake snake) {

	
	for (int i = 0; i < snake.size; i++) {
		gameInfo.boardGame[snake.coords[i].posY][snake.coords[i].posX] = snake.print;
	}
	if (delX >= 0 && delY >= 0) {
		gameInfo.boardGame[delY][delX] = 0;
	}
	
}

Snake move(Snake snake, int move) {
	Coords toMove = snake.coords[0];
	int delX, delY;

	switch (move) {
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
	int positionToCheck = gameInfo.boardGame[toMove.posY][toMove.posX];
	
	// Verify the position to go before move the snake
	switch (positionToCheck) {

		case BLOCK_EMPTY:
			break;
		case BLOCK_WALL:
			// kill the snake
			snake.alive = FALSE;
			snake.print = 0;
			delX = delY = -1;
			return;
			break;
		case BLOCK_FOOD:
			break;
		case BLOCK_ICE:
			break;
		case BLOCK_GRANADE:
			break;
		case BLOCK_VODKA:
			break;
		case BLOCK_OIL:
			break;
		case BLOCK_GLUE:
			break;
		case BLOCK_O_VODKA:
			break;
		case BLOCK_O_OIL:
			break;
		case BLOCK_O_GLUE:
			break;
		default:
			break;
	}

	if (positionToCheck == 5) {
		// kill the snake
		snake.alive = FALSE;
		snake.print = 0;
		delX = delY = -1;
	}
	
	if (snake.alive) {

		// Get position to delete in case of move
		delX = snake.coords[snake.size - 1].posX;
		delY = snake.coords[snake.size - 1].posY;

		// Copy the coordinates of the snake to apply movement 
		for (int i = snake.size - 1; i > 0; i--) {
			snake.coords[i].posX = snake.coords[i - 1].posX;
			snake.coords[i].posY = snake.coords[i - 1].posY;
		}

		// Apply the movement to the head
		switch (move) {
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

	// Draw the snake on the map
	putSnakeIntoBoard(delX, delY, snake);

	return snake;
}


//////////////
//THREADS ////
//////////////

//---------------------------------------------------------
// THREAD RESPONSABLE FOR ATTENDING CLIENTS VIA NAMEDPIPED
//---------------------------------------------------------

DWORD WINAPI listenClientNamedPipes (LPVOID param){

	data request, reply;
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

	_tcscpy(reply.who, TEXT("SERVER"));
	addClient(hPipe);

	while(1){

		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(ReadFromClient);
		overLapped.hEvent = ReadFromClient;

		//READ FROM CLIENT
		success = ReadFile (
					hPipe,		 //READ CHANNEL
					&request,	 //BUFFER OF READING DATA
					DataStructSize,    //SIZE OF STRUCT
					&cbBytesRead,
					&overLapped);

		WaitForSingleObject(ReadFromClient, INFINITE);

		GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE);
		if(cbBytesRead < DataStructSize){
			_tprintf(TEXT("\n[THREAD] ReadFile does not read all - %d"), GetLastError());
		}

		///////////////////////////////////////

		_tprintf(TEXT("[%s] : [%s]\n"), request.who, request.command);

		_tcscpy(reply.command, request.command);
		broadcastClients(reply);
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

	eReadFromClientSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM"));
	eWriteToClientSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM_eWriteToClientSHM"));

	while (1) {

		HANDLE hGameThread;
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

		switch (dataGame.op) {
			case EXIT:
				_tprintf(TEXT("Goodbye.."));

				break;
			case CREATE_GAME:
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
				break;
			case JOIN_GAME:
				break;
			case SCORES:
				break;
			case MOVE_SNAKE:
				diretionToGo = dataGame.direction;
				break;
			default:
				break;
		}
		ResetEvent(eReadFromClientSHM);
	}	
}


//---------------------------------------------------
// GAMING THREAD
//---------------------------------------------------

DWORD WINAPI gameThread(LPVOID params) {
	Snake snake;

	void(*setInfoSHM)();

	_tprintf(TEXT("\n-----GAMETHREAD----\n"));

	//GETDATA IN CORRECT PULL POSITION
	setInfoSHM = (void(*)()) GetProcAddress(hSnakeDll, "setInfoSHM");
	if (setInfoSHM == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading getDataSHM function from DLL (%d)\n"), GetLastError());
		return;
	}

	snake = initSnake(10,10,6);
	initGameInfo();

	while (1) {

		if (diretionToGo != 0 && snake.alive) {
			switch (diretionToGo) {
				case RIGHT:
					if (snake.direction != LEFT) {
						snake = move(snake, RIGHT);
					}
					break;
				case LEFT:
					if (snake.direction != RIGHT) {
						snake = move(snake, LEFT);
					}
					break;
				case UP:
					if (snake.direction != DOWN) {
						snake = move(snake, UP);
					}
					break;
				case DOWN:
					if (snake.direction != UP) {
						snake = move(snake, DOWN);
					}
					break;
				default:
					break;
			}

			_tprintf(TEXT("\n\n"));
			for (int i = 0; i < gameInfo.nRows; i++) {
				for (int j = 0; j < gameInfo.nRows; j++) {
					_tprintf(TEXT(" %d "), gameInfo.boardGame[i][j]);
				}
				_tprintf(TEXT("\n"));
			}
		}
		diretionToGo = 0;

		setInfoSHM(gameInfo);
		SetEvent(eWriteToClientSHM);
		Sleep(0.5*1000);
	}
}


