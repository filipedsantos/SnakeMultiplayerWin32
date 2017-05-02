#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>

#define BUFFSIZE 1024
#define WHOS 60
#define COMMANDSIZE 60

typedef struct data {
	TCHAR sender[WHOS];
	TCHAR command[COMMANDSIZE];

} data;

#define structSize sizeof(data)

int mayContinue = 1;
int readerAlive = 0;

// função auxiliar para ler do caracteres (jduraes)
void readTChars(TCHAR * p, int maxchars) {
	int len;
	_fgetts(p, maxchars, stdin);
	len = _tcslen(p);
	if (p[len - 1] == TEXT('\n')) {
		p[len - 1] = TEXT('\0');
	}
}

DWORD WINAPI ThreadClientReader(LPVOID PARAMS);


///////////////////////////////////// MAIN
int _tmain(int argc, LPTSTR argv[]) {

	HANDLE hPipe;
	BOOL fSucess = FALSE;
	DWORD cbWritten, dwMode;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");

	data dataToSend;
	HANDLE hThread;
	DWORD dwThreadId = 0;


	#ifdef UNICODE
		_setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif

	_tprintf(TEXT("Write your name > "));
	readTChars(dataToSend.sender, WHOS);

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

	HANDLE WriteReady;	// HANDLE para o evento leitura
	OVERLAPPED overLapped = { 0 };

	WriteReady = CreateEvent(
		NULL,		// default security
		TRUE,		// manual reset
		FALSE,		// not signaled
		NULL);		// nao precisa de nome, interno ao processo

	if (WriteReady == NULL) {
		_tprintf(TEXT("[ERROR] Create Event failed... (%d)"), GetLastError());
		return -1;
	}

	_tprintf(TEXT("\nConnected... \"exit\" to stop..."));
	while (1) {
		_tprintf(TEXT("\n%s > "), dataToSend.sender);
		readTChars(dataToSend.command, structSize);

		if (_tcscmp(TEXT("exit"), dataToSend.command) == 0)
			break;

		_tprintf(TEXT("\n Sending %d bytes: \"%s\""), structSize, dataToSend.command);
	

		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(WriteReady);
		overLapped.hEvent = WriteReady;

		fSucess = WriteFile(
			hPipe,				// pipe handle
			&dataToSend,		// message
			structSize,			// message size
			&cbWritten,			// ptr to save number of written bytes
			&overLapped);		// not nul -> not overlapped I/O

		WaitForSingleObject(WriteReady, INFINITE);
		_tprintf(TEXT("\nWrite sucess.."));

		GetOverlappedResult(hPipe, &overLapped, &cbWritten, FALSE); // no wait
		if (cbWritten < structSize) {
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

	CloseHandle(WriteReady);
	CloseHandle(hPipe);
	return 0;
}

DWORD WINAPI ThreadClientReader(LPVOID PARAMS) {
	data fromServer;

	DWORD cbBytesRead = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)PARAMS;	// a info enviada é o handle
	
	HANDLE ReadReady;
	OVERLAPPED overLapped = { 0 };

	if (hPipe == NULL) {
		_tprintf(TEXT("[ERROR] Thread Reader - handle its null..."));
		return -1;
	}

	ReadReady = CreateEvent(
		NULL,		// defalt security
		TRUE,		// resetEvent -> requisito do Overlapped I/O
		FALSE,		// not signaled
		NULL);		// name not needed

	if (ReadReady == NULL) {
		_tprintf(TEXT("[ERROR] Client - read event cannot be created..."));
		return -1;
	}

	readerAlive = 1;
	_tprintf(TEXT("Thread Reader - receiving messages..."));

	while (mayContinue) {

		// prepare ReadReady event
		ZeroMemory(&overLapped, sizeof(overLapped));
		overLapped.hEvent = ReadReady;
		ResetEvent(ReadReady);

		fSuccess = ReadFile(
			hPipe,			// pipe handle (params)
			&fromServer,	// read bytes buffer
			structSize,		// msg size
			&cbBytesRead,	// msg size to read
			&overLapped);	// not null -> not overlapped

		WaitForSingleObject(ReadReady, INFINITE);
		_tprintf(TEXT("\nRead success..."));

		if (!fSuccess || cbBytesRead < structSize) {
			GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE); // No wait
			if (cbBytesRead < structSize) {
				_tprintf(TEXT("[ERROR] Read file failed... (%d)"), GetLastError());
			}
		}

		_tprintf(TEXT("\nServer send: [%s]"), fromServer.command);

	}
	
	readerAlive = 0;
	_tprintf(TEXT("Thread Reader ending\n"));
	return 1;
}