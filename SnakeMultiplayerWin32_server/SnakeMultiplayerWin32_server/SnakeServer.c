#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include <io.h>
#include <fcntl.h>


#define BUFFSIZE 1024

#define WHOS		60
#define COMMANDSIZE 60
#define MAXCLIENTS  20

typedef struct data {

	TCHAR who[WHOS];
	TCHAR command[COMMANDSIZE];

} data;

#define structSize sizeof(data)

DWORD WINAPI listenClient(LPVOID params);
HANDLE clients[MAXCLIENTS];

HANDLE WriteReady;

void readTChars(TCHAR *p, int maxChars){
	int len;
	_fgetts(p, maxChars, stdin);
	len = _tcslen(p);
	if(p[len -1] == TEXT('\n'));
		p[len-1] = TEXT('\0');
}

//START ALL CLIENTS WITH NULL
void startClients(){
	int i = 0;
	
	for(i; i<MAXCLIENTS; i++){
		clients[i] = NULL;
	}

}

//CREATE A NEW CLIENT
void addClient(HANDLE hPipe){
	int i = 0;
	for(i; i<MAXCLIENTS; i++){

		if(clients[i] == NULL){

			clients[i] = hPipe;
			return;
		}
	}
}

//REMOVE CLIENT
void removeClients(HANDLE hPipe){

	int i = 0;
	for(i; i < MAXCLIENTS; i++){
	
		if(clients[i] == hPipe){
			clients[i] = NULL;
			return;
		}
	}
}

//CREATE A MESSAGE TO CLIENT
void writeClients(HANDLE client, data dataReply){
	DWORD cbWritten = 0;
	BOOL  success = FALSE;

	OVERLAPPED overLapped = { 0 };

	ZeroMemory(&overLapped, sizeof(overLapped));
	ResetEvent(WriteReady);
	overLapped.hEvent = WriteReady;

	success = WriteFile(
				client,	
				&dataReply,
				structSize, 
				&cbWritten,
				&overLapped);
	
	WaitForSingleObject(WriteReady, INFINITE);

	GetOverlappedResult(client, &overLapped, &cbWritten, FALSE);
	if(cbWritten < structSize)
		_tprintf(TEXT("\nIncomplete data... Error: %d"), GetLastError());

}

//SEND TO EVERYCLIENT THE SAME MESSAGE
void broadcastClients(data dataReply){
	int i = 0;
	for (i; i < MAXCLIENTS; i++) {
		if (clients[i] != 0) {
			writeClients(clients[i], dataReply);
		}
	}
}


/////////////MAIN 
int _tmain(void){

	BOOL fconnected = FALSE;
	DWORD dwThreadID = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	LPTSTR lpszPipeName = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");

	#ifdef UNICODE
		 _setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif

	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (WriteReady == NULL) {
		_tprintf(TEXT("\n Error creating event write - %d\n"), GetLastError());
		return -1;
	}


	//////MAIN CYCLE OF PROGRAM

	startClients();
	//////-----CREATE NAMEPIPE THEN CREATE THREAD FOR USER

	while(1){
		hPipe = CreateNamedPipe(
			lpszPipeName,			 //name
			PIPE_ACCESS_DUPLEX  |
			FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE    |
			PIPE_READMODE_MESSAGE|
			PIPE_WAIT,				 //mode message-read, blocked
			PIPE_UNLIMITED_INSTANCES,//255 instances
			BUFFSIZE,
			BUFFSIZE, 
			5000,					 //TIMEOUT
			NULL);					 //DEFAULT SECURITY	


		if(hPipe == INVALID_HANDLE_VALUE){
			_tprintf(TEXT("NAMEDPIPED FAILED.... ERROR: %d\n"), GetLastError());
			return -1;
		}

		_tprintf(TEXT("WAIT CLIENT.....\n"));

		fconnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);


		//IF THE CONNECTION HAS BEEN DONE
		if(fconnected){
		
			hThread = CreateThread(
				NULL,
				0,
				listenClient,	
				(LPVOID)hPipe, //PARAM PASSED
				0,
				&dwThreadID);  //ID


			if(hThread == NULL){
				_tprintf(TEXT("\n Error creating a new thread : %d"), GetLastError());
				return -1;
			}
			else
				CloseHandle(hThread); //HANDLE IS NOT NECESSARY
		
		}else{
		
			_tprintf(TEXT("\nClient didn't connect"));
			CloseHandle(hPipe);
		}
	}
		

	return 0;
}

//ATTEND CLIENT
DWORD WINAPI listenClient (LPVOID param){
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
					structSize,  //SIZE OF STRUCT
					&cbBytesRead,
					&overLapped);

		WaitForSingleObject(ReadReady, INFINITE);

		GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE);
		if(cbBytesRead < structSize){
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

