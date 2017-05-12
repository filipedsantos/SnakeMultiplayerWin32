#include <windows.h>
#include <tchar.h>

#include "SnakeDll.h"

static pCircularBuff circularBufferPointer;
static pGameInfo gameInfoPointer;

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

	return TRUE;
}

//DO MAPFILE OF VIEW AND HIS VERIFICATIONS
pCircularBuff getCircularBufferPointerSHM() {
	return circularBufferPointer;
}

pGameInfo getGameInfoPointerSHM() {
	return gameInfoPointer;
}

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
void setDataSHM(pCircularBuff cb, data data, HANDLE mClient, HANDLE semaphoreWrite) {

	//WaitForSingleObject(mClient, INFINITE);
	//WaitForSingleObject(semaphoreWrite, INFINITE);

	// Write on SHM
	cb->circularBuffer[cb->push] = data;

	// INC PUSH
	cb->push = (cb->push + 1) % SIZECIRCULARBUFFER;

	releaseSyncHandles(mClient, semaphoreWrite);
}

//SYNCHRONIZED FUCTION USED TO GETDATA FROM SHARED MEMORY
 data getDataSHM(pCircularBuff pCircularBuff, HANDLE mServer, HANDLE semaphoreRead) {
	 data getData;

	// WaitForSingleObject(mServer, INFINITE);
	// WaitForSingleObject(semaphoreRead, INFINITE);

	 getData = pCircularBuff->circularBuffer[pCircularBuff->pull];

	 pCircularBuff->pull = (pCircularBuff->pull + 1) % SIZECIRCULARBUFFER;

	 releaseSyncHandles(mServer, semaphoreRead);

	 return getData;
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
HANDLE startSyncSemaphore(BOOL writer) {

	HANDLE semaphore;
	int startSize = SIZECIRCULARBUFFER;

	if (!writer) {
		startSize = 0;
	}

	semaphore = CreateSemaphore(
		 NULL,
		 startSize,
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