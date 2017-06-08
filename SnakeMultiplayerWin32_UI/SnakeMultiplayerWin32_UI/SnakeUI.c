#include <windows.h>
#include <tchar.h>
#include "Commctrl.h"

#include "resource.h"
#include "SnakeClient.h"


LRESULT CALLBACK MainWindow(HWND, UINT, WPARAM, LPARAM);
ATOM registerClass(HINSTANCE hInst, TCHAR * szWinName);
HWND CreateMainWindow(HINSTANCE hInst, TCHAR * szWinName);
BOOL CALLBACK DialogTypeUser(HWND, UINT, WPARAM, LPARAM);

void closeEverything();
void startMainWindow();
void startLocal();
void sendCommand(data newData);
void createErrorMessageBox(TCHAR *message);
void createMessageBox(TCHAR *Message);
void updateBoard();


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
HWND hWnd;			// hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
int maxX, maxY;		// Dimens�es do �cran
HBRUSH hbrush;		// Cor de fundo da janela
HDC memdc;			// handler para imagem da janela em mem�ria

GameInfo gameInfo;

static RECT rectangle;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	MSG lpMsg;											// MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;									// WNDCLASSEX é uma estrutura cujos membros servem para 
	BOOL ret;											// definir as características da classe da janela

	windowMode = nCmdShow;
	hThisInst = hInst;

	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por  // "GetStockObject".Neste caso o fundo será branco
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================


	if (!registerClass(hThisInst, WindowName))
		return 0;

	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
	
	hWnd = NULL;
	DialogBox(hThisInst, (LPCSTR)IDD_DIALOG_USER_TYPE, hWnd, (DLGPROC)DialogTypeUser);
	

	// ============================================================================
	// 5. Loop de Mensagens
	// ============================================================================


	while ((ret = GetMessage(&lpMsg, NULL, 0, 0)) != 0) {
		if (ret != -1) {
			TranslateMessage(&lpMsg);	// Pré-processamento da mensagem (p.e. obter código 
										// ASCII da tecla premida)
			DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
										// aguarda até que a possa reenviar à função de 
										// tratamento da janela, CALLBACK TrataEventos (abaixo)
		}
	}


	// ============================================================================
	// 6. Fim do programa
	// ============================================================================

	return((int)lpMsg.wParam);	// Retorna sempre o parâmetro wParam da estrutura lpMsg
}


//----------------------------------------------------
// INIT FUNCTIONS
//----------------------------------------------------

ATOM registerClass(HINSTANCE hInst, TCHAR * szWinName) {
	WNDCLASSEX wcApp;

	wcApp.cbSize = sizeof(WNDCLASSEX);	// Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;			// Instância da janela actualmente exibida 
										// ("hInst" é parâmetro de WinMain e vem 
										// inicializada daí)
	wcApp.lpszClassName = szProgName;	// Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = MainWindow;	// Endereço da função de processamento da janela 	// ("TrataEventos" foi declarada no início e                 // encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;	// Estilo da janela: Fazer o redraw se for      // modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);											// "hIcon" = handler do ícon normal
																							//"NULL" = Icon definido no Windows
																							// "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);// "hIconSm" = handler do ícon pequeno
													//"NULL" = Icon definido no Windows
													// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
													// "NULL" = Forma definida no Windows
													// "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	// (NULL = não tem menu)
	wcApp.cbClsExtra = 0;							// Livre, para uso particular
	wcApp.cbWndExtra = 0;							// Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	return RegisterClassEx(&wcApp);
}

HWND CreateMainWindow(HINSTANCE hInst, TCHAR * szWinName) {

	return CreateWindow(
		szProgName,									// Nome da janela (programa) definido acima
		szWinName,	// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW,						// Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT,								// Posição x pixels (default=à direita da última)
		CW_USEDEFAULT,								// Posição y pixels (default=abaixo da última)
		CW_USEDEFAULT,								// Largura da janela (em pixels)
		CW_USEDEFAULT,								// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,							// handle da janela pai (se se criar uma a partir de

													// outra) ou HWND_DESKTOP se a janela for a primeira, 
													// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da instância do programa actual ("hInst" é 
								// passado num dos parâmetros de WinMain()
		NULL);						// Não há parâmetros adicionais para a janela

}


//----------------------------------------------------
// EVENTS HANDLERS
//----------------------------------------------------

BOOL CALLBACK DialogTypeUser(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
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
	TCHAR aux[1024];
	
	HWND hCombo = NULL;
	HWND hCaption = NULL;
	HWND hEditNickname2 = NULL;

	TCHAR nrPlayers_1[20] = {TEXT(" 1 ")};
	TCHAR nrPlayers_2[20] = {TEXT(" 2 ")};

	switch (messg) {

		case WM_INITDIALOG:

			//COMBO BOX
			hCombo = GetDlgItem(hWnd, IDC_CB_PLAYERS);
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)nrPlayers_1);
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)nrPlayers_2);
			SendDlgItemMessage(hWnd, IDC_CB_PLAYERS, CB_SETCURSEL, 0, 0);


		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case ID_CANCEL_GAME:
					EndDialog(hWnd, 0);
					return 1;


				case ID_CREATE_GAME:

					newData.op = CREATE_GAME;
					GetDlgItemText(hWnd, IDC_EDIT_ROWS, aux, 3);
					newData.nRows = _wtoi(aux);
					GetDlgItemText(hWnd, IDC_EDIT_COLUMNS, aux, 3);
					newData.nColumns = _wtoi(aux);

					sendCommand(newData);


					EndDialog(hWnd, 0);
					updateBoard();
					return 1;
					
					
				//COMBOBOX
				case IDC_CB_PLAYERS:
					
					hEditNickname2 = GetDlgItem(hWnd, IDC_NICKNAME2);
					hCaption = GetDlgItem(hWnd, IDC_CAPTION_NICK);

					if (HIWORD(wParam) == CBN_SELENDOK) {

						GetDlgItemText(hWnd, IDC_CB_PLAYERS, aux, 3);
						
						//SELECTION 2 MAKE 2ND TEXT BOX - ABOUT NICKNAME - APPEAR
						if (_tcscmp(aux, TEXT(" 2")) == 0) {
							ShowWindow(hEditNickname2, SW_SHOWNORMAL);
							ShowWindow(hCaption, SW_SHOWNORMAL);
						}
						else {
							ShowWindow(hEditNickname2, SW_HIDE);
							ShowWindow(hCaption, SW_HIDE);
						}
					}
					return;
			}

	}
	

	return 0;
}

LRESULT CALLBACK MainWindow(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	HDC hdc;						// handler para um Device Context
	HDC auxmemdc;					// handler para Device Context auxiliar em mem�ria
									// que vai conter o bitmap 
	PAINTSTRUCT ps;				// Ponteiro para estrutura de WM_PAINT

	switch (messg) {
		case WM_CREATE:
		{
			SetRect(&rectangle, 1, 1, 20, 20);
			break;
		}

		case WM_KEYDOWN:
		{
			switch (wParam) {
			case VK_LEFT:
				move = LEFT;
				break;

			case VK_RIGHT:
				move = RIGHT;
				break;

			case VK_UP:
				move = UP;
				break;

			case VK_DOWN:
				move = DOWN;
				break;

			default:
				break;
			}
			
			if (!runningThread) {
				runningThread = TRUE;
				hMovementThread = CreateThread(NULL,
					0,
					movementThread,
					NULL,
					0,
					0
				);
			}
		}

		case WM_COMMAND:
		{
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
		}

		case WM_DESTROY:
		{
			// Destruir a janela e terminar o programa 
			// "PostQuitMessage(Exit Status)"		
			closeEverything();
			break;
		}

		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);
			//BitBlt(hdc, 0, 0, maxX, maxY, memdc, 0, 0, SRCCOPY);
			Rectangle(ps.hdc, rectangle.left, rectangle.top, rectangle.right, rectangle.bottom);
			EndPaint(hWnd, &ps);
			break;
		}

		case WM_ERASEBKGND:
		{
			break;
		}
		
		case WM_SIZE:
		{
			break;
		}
			
		default:
		{
			// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar") // não é efectuado nenhum processamento, apenas se segue o "default" do Windows			
			return(DefWindowProc(hWnd, messg, wParam, lParam));
		}
			
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
									// "CreateWindow"; "nCmdShow"= modo de exibição (p.e. 
									// normal/modal); é passado como parâmetro de WinMain()

	UpdateWindow(hWnd);				// Refrescar a janela (Windows envia à janela uma 
									// mensagem para pintar, mostrar dados, (refrescar)… 

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
void sendCommand(data newData) {
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
			createMessageBox(TEXT("START GAME!!!"));
		}
		
		gameInfo = getInfoSHM();
		/*if (getInfoSHM().commandId == MOVE_SNAKE) {
			updateBoard();
		}*/

		ResetEvent(eReadFromServerSHM);

	}

	return 1;
}

void updateBoard() {
	int x = 0, y = 0;

	for (int l = 0; l < gameInfo.nRows; l++) {
		for (int c = 0; c < gameInfo.nColumns; c++) {
			if (gameInfo.boardGame[l][c] == 1) {
				SetRect(&rectangle, x, y, x + 20, y + 20);
				InvalidateRect(NULL, NULL, FALSE);
			}
			x += 20;
		}
		x = 0; 
		y += 20;
	}
	//SetRect(&rectangle, 1, 1, 50, 50);
	
}

DWORD WINAPI movementThread(LPVOID lpParam) {
	data data;
	BOOL START = FALSE;
	data.op = 0;
	data.direction = 0;	

	while (runningThread) {

		data.op = MOVE_SNAKE;
		data.direction = move;

		if (move == RIGHT) {		
			rectangle.left += 20;
			rectangle.right += 20;
			InvalidateRect(NULL, NULL, TRUE);
		}

		if (move == LEFT) {
			rectangle.left -= 20;
			rectangle.right -= 20;
			InvalidateRect(NULL, NULL, TRUE);
		}

		if (move == DOWN) {
			rectangle.top += 20;
			rectangle.bottom += 20;
			InvalidateRect(NULL, NULL, TRUE);
		}

		if (move == UP) {
			rectangle.top -= 20;
			rectangle.bottom -= 20;
			InvalidateRect(NULL, NULL, TRUE);
		}

		_stprintf_s(error, 1024, TEXT("move (%d)"), move);
		createErrorMessageBox(error);

		sendCommand(data);
		Sleep(1000);

	}
	return 0;
}