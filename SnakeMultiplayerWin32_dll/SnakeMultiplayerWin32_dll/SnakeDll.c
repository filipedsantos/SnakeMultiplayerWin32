#include <windows.h>
#include <tchar.h>

#include "SnakeDll.h"

int snakeFunction() {
	return 111;
}

//Function used by server to create a file map
pCircularBuff createFileMapping() {
	HANDLE hMapFile;

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BuffsizeCircularBuff,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Creating File Map Object... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return;
	}

	if(hMapFile == INVALID_HANDLE_VALUE)
		_tprintf(TEXT("[DLL - ERROR] dsfaasdfasdfsadfasdfsadfasdfasdf... (%d)\n"), GetLastError());

	return getSHM(hMapFile, TRUE);
}

//Fuction used by users to access the file map
pCircularBuff openFileMapping() {
	HANDLE hMapFile;

	hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Cannot Open File Mapping... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return;
	}

	return getSHM(hMapFile, FALSE);
}

//DO MAPFILE OF VIEW AND HIS VERIFICATIONS
pCircularBuff getSHM(HANDLE hMapFile, BOOL createNewStruct) {
	pCircularBuff circularBuffPointer;

	circularBuffPointer = (pCircularBuff) MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BuffsizeCircularBuff
	);

	if (circularBuffPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object (circularBuffPointer)... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return;
	}
	
	if (createNewStruct) {
		return createNewCircularBuffer(circularBuffPointer);
	}


	return circularBuffPointer;
}



pCircularBuff createNewCircularBuffer(pCircularBuff cb) {
	
	int i = 0;
	data nullData;

	nullData.op = 0;
	nullData.command[10] = TEXT("fdasfsad");

	for (i; i < SIZECIRCULARBUFFER; i++) {
		cb->circularBuffer[i] = nullData;
	}

	cb->pull = 0;
	cb->push = 0;

	return cb;
}


//SYNCHRONIZED FUCTION USED TO WRITE ON SHARED MEMORY
void setDataSHM(pCircularBuff cb, data data, HANDLE mClient, HANDLE semaphoreWrite) {

	WaitForSingleObject(mClient, INFINITE);
	WaitForSingleObject(semaphoreWrite, INFINITE);

	// Write on SHM
	cb->circularBuffer[cb->push] = data;

	// INC PUSH
	cb->push = (cb->push + 1) % SIZECIRCULARBUFFER;

	releaseSyncHandles(mClient, semaphoreWrite);
}

//SYNCHRONIZED FUCTION USED TO GETDATA FROM SHARED MEMORY
 data getDataSHM(pCircularBuff pCircularBuff, HANDLE mServer, HANDLE semaphoreRead) {
	 data getData;

	 WaitForSingleObject(mServer, INFINITE);
	 WaitForSingleObject(semaphoreRead, INFINITE);

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