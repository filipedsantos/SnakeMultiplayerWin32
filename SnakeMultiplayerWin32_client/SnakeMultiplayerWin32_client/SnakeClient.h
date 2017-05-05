#pragma once

#define BUFFSIZE 1024
#define WHOS 60
#define COMMANDSIZE 60

#define LOCALCLIENT 0
#define REMOTECLIENT 1

typedef struct data {
	TCHAR sender[WHOS];
	TCHAR command[COMMANDSIZE];

} data;

#define structSize sizeof(data)

int typeClient = -1;

void askTypeClient();
void startLocalClient();
void startRemoteClient();
void createGame();

DWORD WINAPI ThreadClientReader(LPVOID PARAMS);

int mayContinue = 1;
int readerAlive = 0;

