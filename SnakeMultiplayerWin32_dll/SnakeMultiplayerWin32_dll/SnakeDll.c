#include <windows.h>
#include <tchar.h>

#include "SnakeDll.h"

static pCircularBuff circularBufferPointer;
static pGameInfo gameInfoPointer;

static HANDLE mClient, mServer;
static HANDLE semaphoreWrite, semaphoreRead;

int snakeFunction() {
	return 111;
}

//Function used by server to create a file map
BOOL createFileMapping() {
	HANDLE hMapFile;

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BuffsizeCircularBuff + GameInfoStructSize,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Creating File Map Object... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}

	if(hMapFile == INVALID_HANDLE_VALUE)
		_tprintf(TEXT("[DLL - ERROR] dsfaasdfasdfsadfasdfsadfasdfasdf... (%d)\n"), GetLastError());

	circularBufferPointer = (pCircularBuff)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BuffsizeCircularBuff
	);

	if (circularBufferPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object (circularBuffPointer)... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}

	gameInfoPointer = (pGameInfo)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		GameInfoStructSize
	);

	if (gameInfoPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object (gameInfoPointer)... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}

	createNewCircularBuffer();

	return TRUE;
}

//Fuction used by users to access the file map
BOOL openFileMapping() {
	HANDLE hMapFile;

	hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Cannot Open File Mapping... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}

	circularBufferPointer = (pCircularBuff)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BuffsizeCircularBuff
	);

	if (circularBufferPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object (circularBuffPointer)... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}

	gameInfoPointer = (pGameInfo)MapViewOfFile(
		hMapFile,
		FILE_MAP_READ,
		0,
		0,
		GameInfoStructSize
	);

	if (gameInfoPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object (gameInfoPointer)... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return FALSE;
	}


	mClient = startSyncMutex();
	semaphoreWrite = startSyncSemaphore(SIZECIRCULARBUFFER);
	return TRUE;
}

//DO MAPFILE OF VIEW AND HIS VERIFICATIONS
//pCircularBuff getCircularBufferPointerSHM() {
//	return circularBufferPointer;
//}
//
//pGameInfo getGameInfoPointerSHM() {
//	return gameInfoPointer;
//}

void createNewCircularBuffer() {
	
	int i = 0;
	data nullData;


	nullData.op = 0;
	nullData.command[10] = TEXT("Empty");

	for (i; i < SIZECIRCULARBUFFER; i++) {
		circularBufferPointer->circularBuffer[i] = nullData;
	}

	circularBufferPointer->pull = 0;
	circularBufferPointer->push = 0;
}

//SYNCHRONIZED FUCTION USED TO WRITE ON SHARED MEMORY
void setDataSHM(data data) {

	WaitForSingleObject(semaphoreWrite, INFINITE);
	WaitForSingleObject(mClient, INFINITE);


	// Write on SHM
	circularBufferPointer->circularBuffer[circularBufferPointer->push] = data;

	// INCREMENT PUSH
	circularBufferPointer->push = (circularBufferPointer->push + 1) % SIZECIRCULARBUFFER;
}

//SYNCHRONIZED FUCTION USED TO GETDATA FROM SHARED MEMORY
 data getDataSHM() {
	 data getData;

	 getData = circularBufferPointer->circularBuffer[circularBufferPointer->pull];

	 circularBufferPointer->pull = (circularBufferPointer->pull + 1) % SIZECIRCULARBUFFER;
	
	 releaseSyncHandles(NULL, semaphoreWrite);


	 return getData;
}

 _declspec(dllexport) void setInfoSHM(GameInfo gi) {
	 gameInfoPointer->commandId = gi.commandId;
	 gameInfoPointer->nRows = gi.nRows;
	 gameInfoPointer->nColumns = gi.nColumns;
	 gameInfoPointer->playerId = gi.playerId;


	 memcpy(gameInfoPointer->boardGame, gi.boardGame, sizeof(int)*100*100);
 }

 _declspec(dllexport) GameInfo getInfoSHM() {
	 GameInfo gi = *gameInfoPointer;
	 return gi;
 }
 
 //STARTING MUTEX
HANDLE startSyncMutex() {

	HANDLE mutex;

	mutex = CreateMutex(NULL, FALSE, NULL);
	if (mutex == NULL) {
		_tprintf(TEXT("[ERROR] semaphore wasn't created... %d"), GetLastError());
		return NULL;
	}

	return mutex;
 }

 //STARTING SYNCHRONIZATION
HANDLE startSyncSemaphore(int sizeOfSemaphore) {
	HANDLE semaphore;

	semaphore = CreateSemaphore(
		 NULL,
		 sizeOfSemaphore,
		 SIZECIRCULARBUFFER,
		 NULL);

	 if (semaphore == NULL) {
		 _tprintf(TEXT("[ERROR] semaphore wasn't created... %d"), GetLastError());
		 return NULL;
	 }

	 return semaphore;
 }

 //RELEASE SYNCHRONIZATION
void releaseSyncHandles(HANDLE mutex, HANDLE semaphore) {

	ReleaseMutex(mutex);
	ReleaseSemaphore(semaphore, 1, NULL);
	
}