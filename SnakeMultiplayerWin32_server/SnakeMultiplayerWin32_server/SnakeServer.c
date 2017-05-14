#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>


#include "SnakeServer.h"


TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

HANDLE clients[MAXCLIENTS];
HANDLE hMapFile;

HANDLE eWriteToClientNP;

HANDLE eReadFromClientSHM;
HANDLE eWriteToClientSHM;

HANDLE hThreadSharedMemory;

HINSTANCE hSnakeDll;


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
		dataSize,
		&cbWritten,
		&overLapped);

	WaitForSingleObject(eWriteToClientNP, INFINITE);

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


////////////////
//THREADS	////
////////////////

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
					dataSize,    //SIZE OF STRUCT
					&cbBytesRead,
					&overLapped);

		WaitForSingleObject(ReadFromClient, INFINITE);

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
	

		switch (getDataSHM().op) {
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

	_tprintf(TEXT("\n-----GAMETHREAD----\n"));

	void(*setInfoSHM)();

	//Wait for any client trigger the event by typing any option
	WaitForSingleObject(eReadFromClientSHM, INFINITE);


	//GETDATA IN CORRECT PULL POSITION
	setInfoSHM = (void(*)()) GetProcAddress(hSnakeDll, "setInfoSHM");
	if (setInfoSHM == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading getDataSHM function from DLL (%d)\n"), GetLastError());
		return;
	}

	GameInfo gi;
	gi.commandId = 222;

	setInfoSHM(gi);

	SetEvent(eWriteToClientSHM);
}

