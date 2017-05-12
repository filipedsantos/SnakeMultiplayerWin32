#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>

#include "SnakeClient.h"

TCHAR readWriteMapName[] = TEXT("fileMappingReadWrite");

HANDLE hMapFile;
HANDLE eWriteToServerSHM;

HANDLE hSnakeDll;

data newData;


// função auxiliar para ler do caracteres (jduraes)
void readTChars(TCHAR * p, int maxchars) {
	int len;
	_fgetts(p, maxchars, stdin);
	len = _tcslen(p);
	if (p[len - 1] == TEXT('\n')) {
		p[len - 1] = TEXT('\0');
	}
}

///////////////////////////////////// MAIN
int _tmain(int argc, LPTSTR argv[]) {

	#ifdef UNICODE
		_setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif

	askTypeClient();

	switch (typeClient) {
		case LOCALCLIENT:
			startLocalClient();
			break;
		case REMOTECLIENT:
			startRemoteClient();
			break;
		default:
			break;
	}

	
	return 0;
}

////////////////
//functions	////
////////////////

void askTypeClient() {

	_tprintf(TEXT("Welcome !!\n\n0 - local\n1 - remote\n\n>> "));
	_tscanf(TEXT("%d"), &typeClient);
}

void startLocalClient() {

	pCircularBuff circularBufferPointer;
	
	// DLL IMPORTED FUNCTIONS - FUNCTION POINTERS
	BOOL(*openFileMap)();
	pCircularBuff(*getCircularBufferPointerSHM)();


	int(*ptr)();

	_tprintf(TEXT("> LOCAL CLIENT\n\n"));

	//LOADING SnakeDll
	hSnakeDll = LoadLibraryEx(TEXT("..\\..\\SnakeMultiplayerWin32_dll\\Debug\\SnakeMultiplayerWin32_dll.dll"), NULL, 0);
	if (hSnakeDll == NULL) {
		_tprintf(TEXT("[ERROR] Dll not available... (%d)\n"), GetLastError());
		return;
	}

	//CHECK DLL FUNCTION
	ptr = (int(*)()) GetProcAddress(hSnakeDll, "snakeFunction");
	if (ptr == NULL) {
		_tprintf(TEXT(">>[SHM ERROR] GetProcAddress\n"));
		return;
	}
	if (ptr() == 111)
		_tprintf(TEXT("DLL correctly Loaded >> ...............%d\n"), ptr());
	else
		_tprintf(TEXT("[ERROR] SERVER probably is not online :(.....(%d)\n"), ptr());

	//OPENFILEMAP
	openFileMap = (BOOL(*)()) GetProcAddress(hSnakeDll, "openFileMapping");
	if (openFileMap == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading createFileMapping function from DLL (%d)\n"), GetLastError());
		return;
	}

	if (!openFileMap()) {
		_tprintf(TEXT("[SHM ERROR] Opening File Map Object... (%d)\n"), GetLastError());
		return;
	 }
	
	getCircularBufferPointerSHM = (pCircularBuff(*)()) GetProcAddress(hSnakeDll, "getCircularBufferPointerSHM");
	if (getCircularBufferPointerSHM == NULL) {
		_tprintf(TEXT("[SHM ERROR] Loading getCircularBufferPointerSHM function from DLL (%d)\n"), GetLastError());
		return;
	}

	circularBufferPointer = getCircularBufferPointerSHM();
	

	//EVENT TO INFORM SERVER THAT SOMETHING WAS CHANGED
	eWriteToServerSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM"));

	newData.op = 0;

	gameMenu(circularBufferPointer);

}

void gameMenu(pCircularBuff p) {
	int op;

	// DLL FUNCTIONS - FUNCTION POINTERS
	void(*setDataSHM)(data);

	_tprintf(TEXT("------ SnakeMultiplayer ------\n\n"));
	_tprintf(TEXT("1 - Create game\n"));
	_tprintf(TEXT("2 - Join game\n"));
	_tprintf(TEXT("3 - Scores\n"));
	_tprintf(TEXT("4 - Settings\n"));
	_tprintf(TEXT("0 - Exit\n"));
	_tprintf(TEXT("\n\n>> "));

	do {
		_tscanf(TEXT("%d"), &op);
	} while (op < 0 || op > 4);

	system("cls");

	switch (op) {
		case 0:
			_tprintf(TEXT("OPTION 0 - EXIT\n"));
			break;
		case 1:
			newData.op = op;

			// GET DLL FUNCTION - setSHM
			setDataSHM = (void(*)(data)) GetProcAddress(hSnakeDll, "setDataSHM");
			if (setDataSHM == NULL) {
				_tscanf(TEXT("[SHM ERROR] GetProcAddress - setDataSHM()\n"));
				return;
			}

			// CALL DLL FUNCTION
			setDataSHM(newData);
			_tprintf(TEXT("OPTION 1 - CREATE GAME\n"));
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		default:
			break;
	}

	SetEvent(eWriteToServerSHM);
}

void createGame() {
	_tprintf(TEXT("Create New Game\n\n"));
}

void startRemoteClient() {
	HANDLE hPipe;
	BOOL fSucess = FALSE;
	DWORD cbWritten, dwMode;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");

	data dataToSend;
	HANDLE hThread;
	DWORD dwThreadId = 0;

	_tprintf(TEXT("Write your name > "));
	//readTChars(dataToSend.sender, WHO);

	while (1) {
		// Open pipe with flag - File_Flag_OVERLAPPED
		hPipe = CreateFile(
			lpszPipename,								// name of pipe
			GENERIC_READ | GENERIC_WRITE,				// read & write access
			0 | FILE_SHARE_READ | FILE_SHARE_WRITE,		// sem -> com partilha
			NULL,										// security -> default
			OPEN_EXISTING,								// open existing
			0 | FILE_FLAG_OVERLAPPED,					// default
			NULL);

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("[ERROR] Create file error and not BUSY...(%d)\n"), GetLastError());
			return -1;
		}

		if (!WaitNamedPipe(lpszPipename, 30000)) {
			_tprintf(TEXT("[ERROR] Waited 30 seconds, exit.."), GetLastError());
			return -1;
		}
	}

	dwMode = PIPE_READMODE_MESSAGE;
	fSucess = SetNamedPipeHandleState(
		hPipe,		// pipe handle
		&dwMode,	// pipe new mode type
		NULL,		// max bytes
		NULL);		// timeout

	if (!fSucess) {
		_tprintf(TEXT("[ERROR] SetNamedPipeHandleState failed... (%d)"), GetLastError());
		return -1;
	}

	hThread = CreateThread(
		NULL,					// security
		0,						// tam da pilha -> default
		ThreadClientReader,		// thread function 
		(LPVOID)hPipe,			// thread = handle
		0,						// make alive
		&dwThreadId);			// ptr para ID da thread

	if (hThread == NULL) {
		_tprintf(TEXT("[ERROR] Creating thread... (%d)"), GetLastError());
		return -1;
	}

	HANDLE eWriteToServerNP;	// HANDLE para o evento leitura
	OVERLAPPED overLapped = { 0 };

	eWriteToServerNP = CreateEvent(
		NULL,		// default security
		TRUE,		// manual reset
		FALSE,		// not signaled
		NULL);		// nao precisa de nome, interno ao processo

	if (eWriteToServerNP == NULL) {
		_tprintf(TEXT("[ERROR] Create Event failed... (%d)"), GetLastError());
		return -1;
	}

	_tprintf(TEXT("\nConnected... \"exit\" to stop..."));
	while (1) {
		//_tprintf(TEXT("\n%s > "), dataToSend.sender);
		readTChars(dataToSend.command, dataSize);

		if (_tcscmp(TEXT("exit"), dataToSend.command) == 0)
			break;

		_tprintf(TEXT("\n Sending %d bytes: \"%s\""), dataSize, dataToSend.command);


		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(eWriteToServerNP);
		overLapped.hEvent = eWriteToServerNP;

		fSucess = WriteFile(
			hPipe,				// pipe handle
			&dataToSend,		// message
			dataSize,			// message size
			&cbWritten,			// ptr to save number of written bytes
			&overLapped);		// not nul -> not overlapped I/O

		WaitForSingleObject(eWriteToServerNP, INFINITE);
		_tprintf(TEXT("\nWrite sucess.."));

		GetOverlappedResult(hPipe, &overLapped, &cbWritten, FALSE); // no wait
		if (cbWritten < dataSize) {
			_tprintf(TEXT("[ERROR] Write File failed... (%d)"), GetLastError());
		}

		if (!fSucess) {
			_tprintf(TEXT("\nWrite File failed... (%d)", GetLastError()));
		}

		_tprintf(TEXT("\nMessage send..."));
	}
	_tprintf(TEXT("\n End listener thread..."));
	mayContinue = 0;	// end readerThread
	if (readerAlive) {
		WaitForSingleObject(hThread, 3000);
		_tprintf(TEXT("Thread reader end or timeout..."));
	}
	_tprintf(TEXT("Client Close Connection and leave..."));

	CloseHandle(eWriteToServerNP);
	CloseHandle(hPipe);
}

////////////////
//THREADS	////
////////////////

//-------------------------------------------------------------
// THREAD RESPONSABLE FOR COMUNICATE WITH SERVER BY NAMEDPIPES
//--------------------------------------------------------------

DWORD WINAPI ThreadClientReader(LPVOID PARAMS) {
	data fromServer;

	DWORD cbBytesRead = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)PARAMS;	// a info enviada é o handle
	
	HANDLE eReadFromServerNP;
	OVERLAPPED overLapped = { 0 };

	if (hPipe == NULL) {
		_tprintf(TEXT("[ERROR] Thread Reader - handle its null..."));
		return -1;
	}

	eReadFromServerNP = CreateEvent(
		NULL,		// defalt security
		TRUE,		// resetEvent -> requisito do Overlapped I/O
		FALSE,		// not signaled
		NULL);		// name not needed

	if (eReadFromServerNP == NULL) {
		_tprintf(TEXT("[ERROR] Client - read event cannot be created..."));
		return -1;
	}

	readerAlive = 1;
	_tprintf(TEXT("Thread Reader - receiving messages..."));

	while (mayContinue) {

		// prepare ReadReady event
		ZeroMemory(&overLapped, sizeof(overLapped));
		overLapped.hEvent = eReadFromServerNP;
		ResetEvent(eReadFromServerNP);

		fSuccess = ReadFile(
			hPipe,			// pipe handle (params)
			&fromServer,	// read bytes buffer
			dataSize,		// msg size
			&cbBytesRead,	// msg size to read
			&overLapped);	// not null -> not overlapped

		WaitForSingleObject(eReadFromServerNP, INFINITE);
		_tprintf(TEXT("\nRead success..."));

		if (!fSuccess || cbBytesRead < dataSize) {
			GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE); // No wait
			if (cbBytesRead < dataSize) {
				_tprintf(TEXT("[ERROR] Read file failed... (%d)"), GetLastError());
			}
		}

		_tprintf(TEXT("\nServer send: [%s]"), fromServer.command);

	}
	
	readerAlive = 0;
	_tprintf(TEXT("Thread Reader ending\n"));
	return 1;
}