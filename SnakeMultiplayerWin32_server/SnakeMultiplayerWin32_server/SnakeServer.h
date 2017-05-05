#pragma once

// Defines
#define BUFFSIZE 1024

#define WHOS		60
#define COMMANDSIZE 60
#define MAXCLIENTS  20

typedef struct data {

	TCHAR who[WHOS];
	TCHAR command[COMMANDSIZE];
	int op;

} data, *pData;


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
