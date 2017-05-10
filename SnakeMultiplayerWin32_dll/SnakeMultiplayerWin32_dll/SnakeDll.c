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
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return;
	}

	if (createNewStruct)
		return createNewCircularBuffer(circularBuffPointer);


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

	_tprintf(TEXT("pull %d"), cb->pull);

	return cb;
}

void setDataSHM(pData data) {

}

void getDataSHM(pData data) {

}