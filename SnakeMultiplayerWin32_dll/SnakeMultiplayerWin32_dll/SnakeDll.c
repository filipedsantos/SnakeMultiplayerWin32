#include <windows.h>
#include <tchar.h>

#include "SnakeDll.h"

int snakeFunction() {
	return 111;
}

HANDLE createFileMapping() {
	HANDLE hMapFile;

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUFFSIZE,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Creating File Map Object... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return NULL;
	}

	return hMapFile;
}

HANDLE openFileMapping() {
	HANDLE hMapFile;

	hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		readWriteMapName
	);

	if (hMapFile == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Cannot Open File Mapping... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return NULL;
	}

	return hMapFile;
}

pData getSHM(HANDLE hMapFile) {
	pData dataPointer;

	dataPointer = (pData)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUFFSIZE
	);

	if (dataPointer == NULL) {
		_tprintf(TEXT("[DLL - ERROR] Accessing File Map Object... (%d)\n"), GetLastError());
		CloseHandle(hMapFile);
		return NULL;
	}

	return dataPointer;
}