#include <windows.h>
#include <tchar.h>
#include "Commctrl.h"

#include "resource.h"
#include "SnakeClient.h"


LRESULT CALLBACK HandleEvents(HWND, UINT, WPARAM, LPARAM);
ATOM registerClass(HINSTANCE hInst, TCHAR * szWinName);
HWND CreateMainWindow(HINSTANCE hInst, TCHAR * szWinName);
BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

void closeEverything();
void startMainWindow();
void startLocal();
void startNewGame(data newData);
void createErrorMessageBox(TCHAR *message);
void createMessageBox(TCHAR *Message);


TCHAR *szProgName = TEXT("Snake Multiplayer");
TCHAR *WindowName = TEXT("Snake Multiplayer");
TCHAR error[1024];
int windowMode = 0;

HANDLE hMapFile;
HANDLE eWriteToServerSHM;
HANDLE eReadFromServerSHM;

HANDLE hSnakeDll;
HANDLE hThreadClientReaderSHM;

HINSTANCE hThisInst;
HWND hWnd;											// hWnd � o handler da janela, gerado mais abaixo por CreateWindow()

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	MSG lpMsg;											// MSG � uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;									// WNDCLASSEX � uma estrutura cujos membros servem para 
	BOOL ret;											// definir as caracter�sticas da classe da janela

	windowMode = nCmdShow;
	hThisInst = hInst;

	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por  // "GetStockObject".Neste caso o fundo ser� branco
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================


	if (!registerClass(hThisInst, WindowName))
		return 0;

	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
	
	hWnd = NULL;
	DialogBox(hThisInst, (LPCSTR)IDD_DIALOG_USER_TYPE, hWnd, (DLGPROC)DialogProc);
	

	// ============================================================================
	// 4. Mostrar a janela
	// ============================================================================



	// ============================================================================
	// 5. Loop de Mensagens
	// ============================================================================
	// O Windows envia mensagens �s janelas (programas). Estas mensagens ficam numa fila de
	// espera at� que GetMessage(...) possa ler "a mensagem seguinte"	
	// Par�metros de "getMessage":
	// 1)"&lpMsg"=Endere�o de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
	//   in�cio de WinMain()):
	//			HWND hwnd		handler da janela a que se destina a mensagem
	//			UINT message		Identificador da mensagem
	//			WPARAM wParam		Par�metro, p.e. c�digo da tecla premida
	//			LPARAM lParam		Par�metro, p.e. se ALT tamb�m estava premida
	//			DWORD time		Hora a que a mensagem foi enviada pelo Windows
	//			POINT pt		Localiza��o do mouse (x, y) 
	// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem //   receber as mensagens para todas as janelas pertencentes � thread actual)
	// 3)C�digo limite inferior das mensagens que se pretendem receber
	// 4)C�digo limite superior das mensagens que se pretendem receber

	// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
	// 	  terminando ent�o o loop de recep��o de mensagens, e o programa 


	while ((ret = GetMessage(&lpMsg, NULL, 0, 0)) != 0) {
		if (ret != -1) {
			TranslateMessage(&lpMsg);	// Pr�-processamento da mensagem (p.e. obter c�digo 
										// ASCII da tecla premida)
			DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
										// aguarda at� que a possa reenviar � fun��o de 
										// tratamento da janela, CALLBACK TrataEventos (abaixo)
		}
	}


	// ============================================================================
	// 6. Fim do programa
	// ============================================================================

	return((int)lpMsg.wParam);	// Retorna sempre o par�metro wParam da estrutura lpMsg
}


//----------------------------------------------------
// INIT FUNCTIONS
//----------------------------------------------------

ATOM registerClass(HINSTANCE hInst, TCHAR * szWinName) {
	WNDCLASSEX wcApp;

	wcApp.cbSize = sizeof(WNDCLASSEX);	// Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;			// Inst�ncia da janela actualmente exibida 
										// ("hInst" � par�metro de WinMain e vem 
										// inicializada da�)
	wcApp.lpszClassName = szProgName;	// Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = HandleEvents;	// Endere�o da fun��o de processamento da janela 	// ("TrataEventos" foi declarada no in�cio e                 // encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;	// Estilo da janela: Fazer o redraw se for      // modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);											// "hIcon" = handler do �con normal
																							//"NULL" = Icon definido no Windows
																							// "IDI_AP..." �cone "aplica��o"
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);// "hIconSm" = handler do �con pequeno
													//"NULL" = Icon definido no Windows
													// "IDI_INF..." �con de informa��o
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
													// "NULL" = Forma definida no Windows
													// "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	// (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0;							// Livre, para uso particular
	wcApp.cbWndExtra = 0;							// Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	return RegisterClassEx(&wcApp);
}

HWND CreateMainWindow(HINSTANCE hInst, TCHAR * szWinName) {

	return CreateWindow(
		szProgName,									// Nome da janela (programa) definido acima
		szWinName,	// Texto que figura na barra do t�tulo
		WS_OVERLAPPEDWINDOW,						// Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT,								// Posi��o x pixels (default=� direita da �ltima)
		CW_USEDEFAULT,								// Posi��o y pixels (default=abaixo da �ltima)
		CW_USEDEFAULT,								// Largura da janela (em pixels)
		CW_USEDEFAULT,								// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,							// handle da janela pai (se se criar uma a partir de

													// outra) ou HWND_DESKTOP se a janela for a primeira, 
													// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da inst�ncia do programa actual ("hInst" � 
								// passado num dos par�metros de WinMain()
		NULL);						// N�o h� par�metros adicionais para a janela

}


//----------------------------------------------------
// EVENTS HANDLERS
//----------------------------------------------------

BOOL CALLBACK DialogProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg) {
		
		case WM_INITDIALOG:
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_BUTTON_LOCAL:
					startMainWindow(hWnd);
					startLocal();
					break;

				case ID_BUTTON_REMOTE:

					startMainWindow(hWnd);
					break;

				default:
					break;
			}

		case WM_DESTROY:
			EndDialog(hWnd, 0);
			return 0;
	}
	
	return 0;
}

BOOL CALLBACK DialogNewGame(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	data newData;
	TCHAR aux[3];

	switch (messg) {
		case WM_INITDIALOG:
			return 1;

		case WM_DESTROY:
			EndDialog(hWnd, 0);
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			
				case ID_CANCEL_GAME:
					EndDialog(hWnd, 0);
					return 1;

				case ID_START_GAME:
					
					newData.op = 1;
					GetDlgItemText(hWnd, IDC_EDIT_ROWS, aux, 3);
					newData.nRows = _wtoi(aux);
					GetDlgItemText(hWnd, IDC_EDIT_COLUMNS, aux, 3);
					newData.nColumns = _wtoi(aux);

					startNewGame(newData);
					return 1;
			}
	}
	

	return 0;
}


LRESULT CALLBACK HandleEvents(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	switch (messg) {
	case WM_COMMAND:

		switch (LOWORD(wParam)) {
			case ID_FILE_EXIT:
				closeEverything();
				break;

			case ID_FILE_NEWGAME:
				DialogBox(hThisInst, (LPCSTR)IDD_DIALOG_NEW_GAME, hWnd, (DLGPROC)DialogNewGame);
				break;

			default:
				break;
		}
		break;

	case WM_DESTROY:	// Destruir a janela e terminar o programa 
						// "PostQuitMessage(Exit Status)"		
		closeEverything();
		break;
	default:

		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar") // n�o � efectuado nenhum processamento, apenas se segue o "default" do Windows			
		return(DefWindowProc(hWnd, messg, wParam, lParam));
	}
	return(0);
}


//----------------------------------------------------
// UTILITY FUNCTIONS
//----------------------------------------------------

void createErrorMessageBox(TCHAR *Message) {
	MessageBox(hWnd, Message, TEXT("ERROR"), MB_ICONERROR);
}

void createMessageBox(TCHAR *Message) {
	MessageBox(hWnd, Message, TEXT("INFO"), MB_ICONINFORMATION);
}

void closeEverything() {

	PostQuitMessage(0);

}


//----------------------------------------------------
// FUNCTIONS
//----------------------------------------------------

//Window created after the first dialog box
void startMainWindow() {

	hWnd = CreateMainWindow(hThisInst, WindowName);
	EndDialog(hWnd, 0);


	ShowWindow(hWnd, windowMode);	// "hWnd"= handler da janela, devolvido por 
									// "CreateWindow"; "nCmdShow"= modo de exibi��o (p.e. 
									// normal/modal); � passado como par�metro de WinMain()

	UpdateWindow(hWnd);				// Refrescar a janela (Windows envia � janela uma 
									// mensagem para pintar, mostrar dados, (refrescar)� 


	

	return;
}

//USER is type --> LOCAL
void startLocal(){

	//EVENT TO INFORM SERVER THAT SOMETHING WAS CHANGED
	eWriteToServerSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM"));
	eReadFromServerSHM = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\snakeMultiplayerSHM_eWriteToClientSHM"));

	pCircularBuff circularBufferPointer;

	// DLL IMPORTED FUNCTIONS - FUNCTION POINTERS
	BOOL(*openFileMap)();
	pCircularBuff(*getCircularBufferPointerSHM)();

	int(*ptr)();

	//LOADING SnakeDll
	hSnakeDll = LoadLibraryEx(TEXT("..\\..\\SnakeMultiplayerWin32_dll\\Debug\\SnakeMultiplayerWin32_dll.dll"), NULL, 0);
	if (hSnakeDll == NULL) {
		_stprintf_s(error, 1024, TEXT("Dll not available... (%d)\n"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

	//CHECK DLL FUNCTION
	ptr = (int(*)()) GetProcAddress(hSnakeDll, "snakeFunction");
	if (ptr == NULL) {
		_stprintf_s(error, 1024, TEXT("[SHM ERROR] GetProcAddress (%d)\n"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

	if (ptr() != 111){
		_stprintf_s(error, 1024, TEXT("SERVER probably is not online :(..... (%d)\n"), GetLastError());
		createErrorMessageBox(error);
		return 0;
	}


	//OPENFILEMAP
	openFileMap = (BOOL(*)()) GetProcAddress(hSnakeDll, "openFileMapping");
	if (openFileMap == NULL) {
		_stprintf_s(error, 1024, TEXT("Loading createFileMapping function from DLL(%d)\n"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

	if (!openFileMap()) {
		_stprintf_s(error, 1024, TEXT("Opening File Map Object... (%d)"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

	// Start thread reader SHM
	hThreadClientReaderSHM = CreateThread(
		NULL,
		0,
		ThreadClientReaderSHM,
		NULL,
		0,
		0
	);

	if (hThreadClientReaderSHM == NULL) {
		_stprintf_s(error, 1024, TEXT("Impossible to create hThreadClientReaderSHM... (%d)"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

}

//Starting a new Game
void startNewGame(data newData) {
	void(*setDataSHM)(data);


	// GET DLL FUNCTION - setSHM
	setDataSHM = (void(*)(data)) GetProcAddress(hSnakeDll, "setDataSHM");
	if (setDataSHM == NULL) {
		_stprintf_s(error, 1024, TEXT("[SHM ERROR] GetProcAddress - setDataSHM()"), GetLastError());
		createErrorMessageBox(error);
		return;
	}

	// CALL DLL FUNCTION
	setDataSHM(newData);

	SetEvent(eWriteToServerSHM);
}

//----------------------------------------------------
// THREADS
//----------------------------------------------------

//THREAD RESPONSABLE FOR LOCAL USERS
DWORD WINAPI ThreadClientReaderSHM(LPVOID PARAMS) {

	GameInfo(*getInfoSHM)();
	while (1) {

		//Wait for any client trigger the event by typing any option
		WaitForSingleObject(eReadFromServerSHM, INFINITE);


		//GETDATA IN CORRECT PULL POSITION
		getInfoSHM = (GameInfo(*)()) GetProcAddress(hSnakeDll, "getInfoSHM");
		if (getInfoSHM == NULL) {
			_stprintf_s(error, 1024, TEXT("[SHM ERROR] Loading getDataSHM function from DLL (%d)"), GetLastError());
			createErrorMessageBox(error);
			return;
		}

		if (getInfoSHM().commandId == 222) {
			createMessageBox(TEXT("SERVER: LETS START A GAME"));
		}

		ResetEvent(eReadFromServerSHM);

	}

	return 1;
}