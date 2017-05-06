#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include <io.h>
#include <fcntl.h>

#include "SnakeServer.h"

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

HANDLE clients[MAXCLIENTS];
HANDLE WriteReady;
HANDLE hMapFile;
HANDLE hThreadSharedMemory;
HANDLE CommandEvent;

/////////////MAIN 
int _tmain(void){

	#ifdef UNICODE
		 _setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif
		
	initializeServer();
		

	return 0;
}	// END MAIN

// Functions ------------------------------------------------------------------------------------

//START ALL CLIENTS WITH NULL
void startClients() {
	int i = 0;

	for (i; i<MAXCLIENTS; i++) {
		clients[i] = NULL;
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

//CREATE A MESSAGE TO CLIENT
void writeClients(HANDLE client, data dataReply) {
	DWORD cbWritten = 0;
	BOOL  success = FALSE;

	OVERLAPPED overLapped = { 0 };

	ZeroMemory(&overLapped, sizeof(overLapped));
	ResetEvent(WriteReady);
	overLapped.hEvent = WriteReady;

	success = WriteFile(
		client,
		&dataReply,
		dataSize,
		&cbWritten,
		&overLapped);

	WaitForSingleObject(WriteReady, INFINITE);

	GetOverlappedResult(client, &overLapped, &cbWritten, FALSE);
	if (cbWritten < dataSize)
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

void initializeServer() {

	HINSTANCE hSnakeDll;
	int(*ptr)();

	// Try to load SnakeDll
	hSnakeDll = LoadLibraryEx(TEXT("SnakeMultiplayerWin32_dll.dll"), NULL, 0);
	if (hSnakeDll == NULL) {
		_tprintf(TEXT("[ERROR] Dll not available... (%d)\n"), GetLastError());
		return;
	}

	ptr = (int(*)(int)) GetProcAddress(hSnakeDll, "snakeFunction");
	if (ptr == NULL) {
		_tprintf(TEXT(">> GetProcAddress\n"));
		return;
	}

	_tprintf(TEXT("WORKk?? >> %d\n"), ptr());

	initializeSharedMemory();
	initializeNamedPipes();

	FreeLibrary(hSnakeDll);
}

void initializeSharedMemory() {
	data * dataPointer;

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUFFSIZE,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[ERROR] Creating File Map Object... (%d)"), GetLastError());
		return;
	}

	dataPointer = (pData)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUFFSIZE
	);

	if (dataPointer == NULL) {
		CloseHandle(hMapFile);
		_tprintf(TEXT("[ERROR] Accessing File Map Object... (%d)"), GetLastError());

	}

	hThreadSharedMemory = CreateThread(
		NULL,
		0,
		listenClientSharedMemory,
		(LPVOID)dataPointer,
		0,
		0
	);

	if (hThreadSharedMemory == NULL) {
		_tprintf(TEXT("[ERROR] Creating Shared Memory Thread... (%d)"), GetLastError());
		return;
	}

	_tprintf(TEXT("Shared Memory initialised\n"));


}

void initializeNamedPipes() {
	BOOL fconnected = FALSE;
	DWORD dwThreadID = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	LPTSTR lpszPipeName = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");



	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (WriteReady == NULL) {
		_tprintf(TEXT("\n Error creating event write - %d\n"), GetLastError());
		return -1;
	}


	//////MAIN CYCLE OF PROGRAM

	startClients();
	//////-----CREATE NAMEPIPE THEN CREATE THREAD FOR USER

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

		_tprintf(TEXT("WAIT CLIENT.....\n"));

		fconnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);


		//IF THE CONNECTION HAS BEEN DONE
		if (fconnected) {

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
				CloseHandle(hThread); //HANDLE IS NOT NECESSARY

		}
		else {

			_tprintf(TEXT("\nClient didn't connect"));
			CloseHandle(hPipe);
		}
	}
}

//ATTEND CLIENT
DWORD WINAPI listenClientNamedPipes (LPVOID param){
	data request, reply;
	BOOL success = FALSE;
	DWORD cbBytesRead  = 0;
	DWORD cbBytesReply = 0;

	HANDLE hPipe = (HANDLE) param; //param received is a handle of the pipe
	OVERLAPPED overLapped = { 0 };

	HANDLE ReadReady;

	//TEST PIPE RECEIVED
	if(hPipe == NULL){
		_tprintf(TEXT("\n [THREAD]: Handle is null %d"), GetLastError());
		return -1;
	}

	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(ReadReady == NULL){
		_tprintf(TEXT("\nError creating Read event - %d"), GetLastError());
		return -1;
	}

	_tcscpy(reply.who, TEXT("SERVER"));
	addClient(hPipe);

	while(1){

		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(ReadReady);
		overLapped.hEvent = ReadReady;

		//READ FROM CLIENT
		success = ReadFile (
					hPipe,		 //READ CHANNEL
					&request,	 //BUFFER OF READING DATA
					dataSize,  //SIZE OF STRUCT
					&cbBytesRead,
					&overLapped);

		WaitForSingleObject(ReadReady, INFINITE);

		GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE);
		if(cbBytesRead < dataSize){
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


	_tprintf(TEXT("\n Thread terminated...."));
	return 0;
}

DWORD WINAPI listenClientSharedMemory(LPVOID params) {

	data * dataPointer = (pData)params;

	CommandEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\cenasfixes"));

	while (1) {
		HANDLE hGameThread;

		WaitForSingleObject(CommandEvent, INFINITE);
		FlushFileBuffers(dataPointer);

		system("cls");

		// Code Here
		switch (dataPointer->op) {
			case EXIT:
				_tprintf(TEXT("Goodbye.."));
				break;
			case CREATE_GAME:
				hGameThread = CreateThread(
					NULL,
					0,
					gameThread,
					(LPVOID)dataPointer,
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
			default:
				break;
		}

		ResetEvent(CommandEvent);
	}
	
}

// Game Thread 
DWORD WINAPI gameThread(LPVOID params) {
	Snake snake;

	snake.coords[0].posX = 5;
	snake.coords[0].posY = 5;

	snake.coords[2].posX = 4;
	snake.coords[2].posY = 5;

	snake.coords[3].posX = 3;
	snake.coords[3].posY = 4;

	snake.draw = '*';

	for (int l = 0; l < 10; l++) {
		for (int c = 0; c < 10; c++) {
			_tprintf(TEXT(" "));
		}
	}
}

