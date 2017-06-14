#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <time.h>


#include "Commctrl.h"
#include "resource.h"
#include "SnakeClient.h"

HANDLE hUserToken = NULL;
BOOL log;
int ret;
TCHAR	username[20],
pass[20],
domain[20];

TCHAR *szProgName = TEXT("Snake Multiplayer");
TCHAR *WindowName = TEXT("Snake Multiplayer");
TCHAR error[1024];
int windowMode = 0;

BOOL cannotCreate = FALSE;

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
HDC hdc;	

HBITMAP hbitGround;
HBITMAP hbitSnake;
HBITMAP hbitSnake2;
HBITMAP hbitSnakeEnemy;
HBITMAP hbitApple;
HBITMAP hbitwall;

HBITMAP hbitIce;
HBITMAP hbitGranade;
HBITMAP hbitOil;
HBITMAP hbitVodka;
HBITMAP hbitGlue;
HBITMAP hbitOOil;
HBITMAP hbitOVodka;
HBITMAP hbitOGlue;


GameInfo gameInfo;

TCHAR keyLeft  = TEXT('A');
TCHAR keyRight = TEXT('D');
TCHAR keyUp    = TEXT('W');
TCHAR keyDown  = TEXT('S');

int myId;
int myId2;

int typeClient = -1; // LOCAL OR REMOTE

// Remote Client Vars
DWORD cbWritten;
HANDLE hPipe;
BOOL fSucess = FALSE;


DWORD dwMode;

HANDLE hThread;
DWORD dwThreadId = 0;
LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\SnakeMultiplayerPipe");
LPTSTR lpszPipeRemoteName;

BOOL running = TRUE;	// updateBoard while loop controler

int objects[9] = {41, 10, 10, 10, 10, 10, 3, 3, 3 };

//MAIN
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	MSG lpMsg;											// MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;									// WNDCLASSEX é uma estrutura cujos membros servem para 
	BOOL ret;											// definir as características da classe da janela

	windowMode = nCmdShow;
	hThisInst = hInst;

	// create an id for user
	srand(time(NULL));
	myId = rand() % 1000 + 1999;
	myId2 = rand() % 1000 + 1999;
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
// WINDOW AND DIALOG
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
					typeClient = LOCALCLIENT;
					startLocal();
					break;

				case ID_BUTTON_REMOTE:
					startMainWindow(hWnd);
					typeClient = REMOTECLIENT;
					startRemote();
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
	TCHAR getText[1024];
	int nPlayers;
	
	HWND hCombo = NULL;
	HWND hRadioSinglePlayer;
	HWND hRadioMultiPlayer;
	HWND hCaption = NULL;
	HWND hEditNickname2 = NULL;

	switch (messg) {

		case WM_INITDIALOG:
			// Radio Buttons
			CheckRadioButton(hWnd, IDC_RADIO_SINGLEPLAYER, IDC_RADIO_MULTIPLAYER, IDC_RADIO_SINGLEPLAYER);
			
			//COMBO BOX
			SendDlgItemMessage(hWnd, IDC_CB_PLAYERS, CB_ADDSTRING, 0, TEXT("1"));
			SendDlgItemMessage(hWnd, IDC_CB_PLAYERS, CB_ADDSTRING, 0, TEXT("2"));
			SendDlgItemMessage(hWnd, IDC_CB_PLAYERS, CB_SETCURSEL, 0, 0);

			// Game Variables
			SendDlgItemMessage(hWnd, IDC_EDIT_GAME_OBJECTS, EM_REPLACESEL, 0, TEXT("10"));
			SendDlgItemMessage(hWnd, IDC_EDIT_OBJECTS_DURATION, EM_REPLACESEL, 0, TEXT("10"));
			SendDlgItemMessage(hWnd, IDC_EDIT_SERPENT_SIZE, EM_REPLACESEL, 0, TEXT("3"));
			SendDlgItemMessage(hWnd, IDC_EDIT_AI_SERPENTS, EM_REPLACESEL, 0, TEXT("0"));


			// ROWS AND COLUMNS
			SendDlgItemMessage(hWnd, IDC_EDIT_ROWS, EM_REPLACESEL, 0, TEXT("30"));
			SendDlgItemMessage(hWnd, IDC_EDIT_COLUMNS, EM_REPLACESEL, 0, TEXT("30"));

			SendDlgItemMessage(hWnd, IDC_NICKNAME1, EM_REPLACESEL, 0, TEXT("MARIO"));

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case WM_DESTROY:
					EndDialog(hWnd, 0);
					return 1;

				case ID_CANCEL_GAME:
					EndDialog(hWnd, 0);
					return 1;


				case ID_CREATE_GAME:
					// COMMAND ID
					newData.op = CREATE_GAME;
					// SINGLE OR MULTIPLAYER
					if (IsDlgButtonChecked(hWnd, IDC_RADIO_SINGLEPLAYER)) {
						newData.typeOfGame = SINGLEPLAYER;
					}
					else {
						newData.typeOfGame = MULTIPLAYER;
					}

					// NUM LOCAL PLAYERS
					GetDlgItemText(hWnd, IDC_CB_PLAYERS, getText, TCHAR_SIZE);
					if (_tcscmp(getText, TEXT("1")) == 0) {
						nPlayers = 1;
					}
					else {
						nPlayers = 2;
					}
					newData.numLocalPlayers = nPlayers;

					// NCKNAME1
					GetDlgItemText(hWnd, IDC_NICKNAME1, getText, TCHAR_SIZE);
					_tcscpy(newData.nicknamePlayer1, getText);

					// NCKNAME2
					if (newData.numLocalPlayers == 2) {
						GetDlgItemText(hWnd, IDC_NICKNAME2, getText, TCHAR_SIZE);
						_tcscpy(newData.nicknamePlayer2, getText);

						// create an id for 2 user
						
					}

					// Number GAME OBJECTS
					GetDlgItemText(hWnd, IDC_EDIT_GAME_OBJECTS, getText, 3);
					newData.gameObjects = _wtoi(getText);
					for (int i = 0; i < 9; i++) {
						newData.objects[i] = objects[i];
					}
					 
					// OBJECTS DURATION
					GetDlgItemText(hWnd, IDC_EDIT_OBJECTS_DURATION, getText, 3);
					newData.objectsDuration = _wtoi(getText);

					// SERPENTS INITIAL SIZE
					GetDlgItemText(hWnd, IDC_EDIT_SERPENT_SIZE, getText, 2);
					newData.serpentInitialSize= _wtoi(getText);

					// AI SERPENTS
					GetDlgItemText(hWnd, IDC_EDIT_AI_SERPENTS, getText, 3);
					newData.AIserpents = _wtoi(getText);

					// BOARD ROWS
					GetDlgItemText(hWnd, IDC_EDIT_ROWS, getText, 3);
					newData.nRows = _wtoi(getText);

					// BOARD COLUMNS
					GetDlgItemText(hWnd, IDC_EDIT_COLUMNS, getText, 3);
					newData.nColumns = _wtoi(getText);

					sendCommand(newData);
					
					EndDialog(hWnd, 0);

					if (!cannotCreate) {
						MessageBox(hWnd, TEXT("Press OK to begin the game"), TEXT("StartGame"), MB_OK);

						newData.op = START_GAME;
						sendCommand(newData);
					}
					else {
						createErrorMessageBox(TEXT("Game already created.."));
					}

					return 1;
					
					
				//COMBOBOX
				case IDC_CB_PLAYERS:
					//hEditNickname2 = GetDlgItem(hWnd, IDC_NICKNAME2);
					//hCaption = GetDlgItem(hWnd, IDC_CAPTION_NICK);
					if (HIWORD(wParam) == CBN_SELENDOK) {

						GetDlgItemText(hWnd, IDC_CB_PLAYERS, getText, 2);
						
						//SELECTION 2 MAKE 2ND TEXT BOX - ABOUT NICKNAME - APPEAR
						if (_tcscmp(getText, TEXT("2")) == 0) {
							ShowWindow(GetDlgItem(hWnd, IDC_NICKNAME2), SW_SHOWNORMAL);
							ShowWindow(GetDlgItem(hWnd, IDC_CAPTION_NICK), SW_SHOWNORMAL);
						}
						else {
							ShowWindow(GetDlgItem(hWnd, IDC_NICKNAME2), SW_HIDE);
							ShowWindow(GetDlgItem(hWnd, IDC_CAPTION_NICK), SW_HIDE);
						}
					}
					return 1;
			}

	}
	

	return 0;
}

BOOL CALLBACK DialogJoinGame(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	data newData;
	TCHAR getText[1024];
	int nPlayers;

	switch (messg) {
	case WM_DESTROY:
		EndDialog(hWnd, 0);
		return 0;

	case WM_INITDIALOG:
		//COMBO BOX
		SendDlgItemMessage(hWnd, IDC_CB_PLAYERS2, CB_ADDSTRING, 0, TEXT("1"));
		SendDlgItemMessage(hWnd, IDC_CB_PLAYERS2, CB_ADDSTRING, 0, TEXT("2"));
		SendDlgItemMessage(hWnd, IDC_CB_PLAYERS2, CB_SETCURSEL, 0, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDJOIN:
			// NUM LOCAL PLAYERS
			GetDlgItemText(hWnd, IDC_CB_PLAYERS, getText, TCHAR_SIZE);
			if (_tcscmp(getText, TEXT("1")) == 0) {
				nPlayers = 1;
			}
			else {
				nPlayers = 2;
			}
			newData.numLocalPlayers = nPlayers;

			// NCKNAME1
			GetDlgItemText(hWnd, IDC_EDIT_PLAYER1, getText, TCHAR_SIZE);
			_tcscpy(newData.nicknamePlayer1, getText);

			// NCKNAME2
			if (newData.numLocalPlayers == 2) {
				GetDlgItemText(hWnd, IDC_EDIT_PLAYER2, getText, TCHAR_SIZE);
				_tcscpy(newData.nicknamePlayer2, getText);
			}

			sendCommand(newData);
			EndDialog(hWnd, 0);
			MessageBox(hWnd, TEXT("Waiting for game start.."),TEXT("Join Game"), MB_ICONINFORMATION);
			return 1;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return 0;

		}

		//COMBOBOX
	case IDC_CB_PLAYERS:
		if (HIWORD(wParam) == CBN_SELENDOK) {

			GetDlgItemText(hWnd, IDC_CB_PLAYERS2, getText, 2);

			//SELECTION 2 MAKE 2ND TEXT BOX - ABOUT NICKNAME - APPEAR
			if (_tcscmp(getText, TEXT("2")) == 0) {
				ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PLAYER2), SW_SHOWNORMAL);
				ShowWindow(GetDlgItem(hWnd, IDC_STATIC_PLAYER2), SW_SHOWNORMAL);
			}
			else {
				ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PLAYER2), SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_STATIC_PLAYER2), SW_HIDE);
			}
		}
		return 1;

	}

	return 0;
}

BOOL CALLBACK DialogEditControls(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[2];
	switch (messg) {

		case WM_INITDIALOG:
			_stprintf(str, TEXT("%c"), keyLeft);
			SendDlgItemMessage(hWnd, IDC_P2_LEFT, EM_REPLACESEL, 0, str);
			_stprintf(str, TEXT("%c"), keyUp);
			SendDlgItemMessage(hWnd, IDC_P2_UP, EM_REPLACESEL, 0, str);
			_stprintf(str, TEXT("%c"), keyRight);
			SendDlgItemMessage(hWnd, IDC_P2_RIGHT, EM_REPLACESEL, 0, str);
			_stprintf(str, TEXT("%c"), keyDown);
			SendDlgItemMessage(hWnd, IDC_P2_DOWN, EM_REPLACESEL, 0, str);
			return 1;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
			case IDOK:
				GetDlgItemText(hWnd, IDC_P2_LEFT, str, 2);
				keyLeft = _toupper(str[0]);
				GetDlgItemText(hWnd, IDC_P2_UP, str, 2);
				keyUp = _toupper(str[0]);
				GetDlgItemText(hWnd, IDC_P2_RIGHT, str, 2);
				keyRight = _toupper(str[0]);
				GetDlgItemText(hWnd, IDC_P2_DOWN, str, 2);
				keyDown = _toupper(str[0]);
				EndDialog(hWnd, 0);
				return 1;
			case IDCANCEL:
				EndDialog(hWnd, 0);
				return 1;
			}
		}

	}

	return 0;
}

BOOL CALLBACK DialogObjects(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	TCHAR tmp[10];
	LRESULT result;
	int objectAux[9];
	int counter = 0;

	switch (messg) {

		case WM_INITDIALOG:

			for (size_t i = 0; i < 9; i++){
				objectAux[i] = objects[i];
			}

			// Food
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_FOOD), TBM_SETPOS, (WPARAM)TRUE, objectAux[0]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[0]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_FOOD), WM_SETTEXT, 0, (LPARAM)tmp);

			// ICE
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_ICE), TBM_SETPOS, (WPARAM)TRUE, objectAux[1]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[1]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_ICE), WM_SETTEXT, 0, (LPARAM)tmp);

			// GRANADE
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GRANADE), TBM_SETPOS, (WPARAM)TRUE, objectAux[2]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[2]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_GRANADE), WM_SETTEXT, 0, (LPARAM)tmp);

			// VODKA
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_VODKA), TBM_SETPOS, (WPARAM)TRUE, objectAux[3]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[3]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_VODKA), WM_SETTEXT, 0, (LPARAM)tmp);

			// OIL
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_OIL), TBM_SETPOS, (WPARAM)TRUE, objectAux[4]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[4]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_OIL), WM_SETTEXT, 0, (LPARAM)tmp);

			// GLUE
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GLUE), TBM_SETPOS, (WPARAM)TRUE, objectAux[5]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[5]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_GLUE), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-VODKA
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_VODKA), TBM_SETPOS, (WPARAM)TRUE, objectAux[6]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[6]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_VODKA), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-OIL
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_OIL), TBM_SETPOS, (WPARAM)TRUE, objectAux[7]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[7]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_OIL), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-GLUE
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_GLUE), TBM_SETPOS, (WPARAM)TRUE, objectAux[8]);
			_stprintf(tmp, TEXT("%d %%"), objectAux[8]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_GLUE), WM_SETTEXT, 0, (LPARAM)tmp);

			break;
		
		case WM_HSCROLL:
			// Food
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_FOOD), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[0] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[0]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_FOOD), WM_SETTEXT, 0, (LPARAM)tmp);

			// ICE
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_ICE), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[1] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[1]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_ICE), WM_SETTEXT, 0, (LPARAM)tmp);

			// GRANADE
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GRANADE), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[2] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[2]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_GRANADE), WM_SETTEXT, 0, (LPARAM)tmp);

			// VODKA
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_VODKA), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[3] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[3]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_VODKA), WM_SETTEXT, 0, (LPARAM)tmp);

			// OIL
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_OIL), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[4] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[4]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_OIL), WM_SETTEXT, 0, (LPARAM)tmp);

			// GLUE
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GLUE), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[5] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[5]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_GLUE), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-VODKA
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_VODKA), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[6] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[6]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_VODKA), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-OIL
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_OIL), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[7] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[7]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_OIL), WM_SETTEXT, 0, (LPARAM)tmp);

			// O-GLUE
			result = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_GLUE), TBM_GETPOS, (WPARAM)0, 0);
			objectAux[8] = result;
			_stprintf(tmp, TEXT("%d %%"), objectAux[8]);
			SendMessage(GetDlgItem(hWnd, IDC_LABEL_O_GLUE), WM_SETTEXT, 0, (LPARAM)tmp);

			break;
			
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				
				case IDOK:
					// Food
					objectAux[0] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_FOOD), TBM_GETPOS, (WPARAM)0, 0);

					// ICE
					objectAux[1] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_ICE), TBM_GETPOS, (WPARAM)0, 0);
					
					// GRANADE
					objectAux[2] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GRANADE), TBM_GETPOS, (WPARAM)0, 0);

					// VODKA
					objectAux[3] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_VODKA), TBM_GETPOS, (WPARAM)0, 0);

					// OIL
					objectAux[4] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_OIL), TBM_GETPOS, (WPARAM)0, 0);

					// GLUE
					objectAux[5] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_GLUE), TBM_GETPOS, (WPARAM)0, 0);

					// O-VODKA
					objectAux[6] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_VODKA), TBM_GETPOS, (WPARAM)0, 0);

					// O-OIL
					objectAux[7] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_OIL), TBM_GETPOS, (WPARAM)0, 0);

					// O-GLUE
					objectAux[8] = SendMessage(GetDlgItem(hWnd, IDC_SLIDER_O_GLUE), TBM_GETPOS, (WPARAM)0, 0);
					
					counter = 0;
					for (int i = 0; i < 9; i++){
						counter += objectAux[i];
					}
					if (counter <= 100) {
						for (int i = 0; i < 9; i++) {
							objects[i] = objectAux[i];
						}
						EndDialog(hWnd, 0);
					}
					break;

				case IDCANCEL:
					EndDialog(hWnd, 0);
					return 1;
			}
			
		}
	return 0;
}

LRESULT CALLBACK MainWindow(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	data data;
	HDC auxmemdc;					// handler para Device Context auxiliar em mem�ria
									// que vai conter o bitmap 
	PAINTSTRUCT ps;				// Ponteiro para estrutura de WM_PAINT
	
	// Double Buffer
	HDC hdcDB;
	HBITMAP hdDB;

	switch (messg) {
		case WM_CREATE:
		{
			//SetRect(&rectangle, 1, 1, 20, 20);


			hdc = GetDC(hWnd);
			memdc = CreateCompatibleDC(hdc);
			// Create background
			hbitGround = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitGround);
			// Create snake
			hbitSnake = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitSnake);
			// Create snake2
			hbitSnake2 = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitSnake2);
			// Create snake enemy
			hbitSnakeEnemy = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitSnakeEnemy);
			// Create food
			hbitApple = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitApple);
			// Create wall
			hbitwall = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitwall);
			
			// Create Ice
			hbitIce = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitIce);
			// Create Granade
			hbitGranade = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitGranade);
			// Create Oil
			hbitOil = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitOil);
			// Create Vodka
			hbitVodka = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitVodka);
			// Create Glue
			hbitGlue = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitGlue);
			// Create O-Oil
			hbitOOil = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitOOil);
			// Create O-Vodka
			hbitOVodka = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitOVodka);
			// Create O-GLue
			hbitOGlue = CreateCompatibleBitmap(hdc, 800, 650);
			SelectObject(memdc, hbitOGlue);

			hbrush = GetStockObject(WHITE_BRUSH);				
			SelectObject(memdc, hbrush);
			PatBlt(memdc, 0, 0, 800, 650, PATCOPY);

			ReleaseDC(hWnd, hdc);

			hbitGround = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_GRASS));
			hbitSnake = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_SNAKE_YELLOW));
			hbitSnake2 = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_SNAKE_GRAY));
			hbitSnakeEnemy = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_SNAKE_PURPLE));
			hbitApple = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_APPLE));
			hbitwall = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_WALL1));
			hbitIce = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_ICE));
			hbitGranade = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_GRANADE));
			hbitOil = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_OIL));
			hbitVodka = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_VODKA));
			hbitGlue = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_GLUE));
			hbitOOil = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_OIL2));
			hbitOVodka = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_VODKA2));
			hbitOGlue = LoadBitmap(hThisInst, MAKEINTRESOURCE(IDB_GLUE));
			break;
		}

		case WM_KEYUP:
		{
			runningThread = FALSE;
			break;
		}

		case WM_KEYDOWN:
		{
			data.op = MOVE_SNAKE;
			switch (wParam) {
				
				case VK_LEFT:
					data.direction = LEFT;
					break;

				case VK_RIGHT:
					data.direction = RIGHT;
					break;

				case VK_UP:
					data.direction = UP;
					break;

				case VK_DOWN:
					data.direction = DOWN;
					break;
				default:
					if (wParam == keyLeft) {
						data.op = MOVE_SNAKE2;
						data.direction = LEFT;
					}
					if (wParam == keyUp) {
						data.op = MOVE_SNAKE2;
						data.direction = UP;
					}
					if (wParam == keyRight) {
						data.op = MOVE_SNAKE2;
						data.direction = RIGHT;
					}

					if (wParam == keyDown) {
						data.op = MOVE_SNAKE2;
						data.direction = DOWN;
					}
					break;
			}

			
			
			sendCommand(data);
			break;

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

				case ID_FILE_JOINGAME:
					//DialogBox(hThisInst, (LPCSTR)IDD_DIALOG_JOIN_GAME, hWnd, (DLGPROC)DialogJoinGame);

					data.op = JOIN_GAME;
					sendCommand(data);
					break;
					
				case ID_SETTINGS_CONTROLS:
					DialogBox(hThisInst, (LPCSTR)IDD_EDIT_CONTROLS, hWnd, (DLGPROC)DialogEditControls);
					break;
				case ID_SETTINGS_OBJECTS:
					DialogBox(hThisInst, (LPCSTR)IDD_OBJECTS, hWnd, (DLGPROC)DialogObjects);
					break;
				case ID_EDITBITMAP_SNAKE:
					editResourceOnPaint(IDB_SNAKE_YELLOW);
					break;
				case ID_EDITBITMAP_WALL:
					editResourceOnPaint(IDB_WALL1);
					break;
				case ID_EDITBITMAP_FOOD:
					editResourceOnPaint(IDB_APPLE);
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

			//// Double Buffer
			//if (hdcDB == NULL) {
			//	hdcDB = CreateCompatibleDC(hdc);
			//	hdDB = CreateCompatibleBitmap(hdc, 800, 650);
			//}

			//int x = 0, y = 0;
			//for (int l = 0; l < gameInfo.nRows; l++) {
			//	for (int c = 0; c < gameInfo.nColumns; c++) {

			//		switch (gameInfo.boardGame[l][c]) {
			//		case BLOCK_EMPTY:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitGround, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_FOOD:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitApple, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_WALL:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitwall, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_ICE:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitIce, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_GRANADE:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitGranade, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_OIL:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitOil, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_VODKA:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitVodka, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_GLUE:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitGlue, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_O_OIL:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitOOil, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_O_VODKA:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitOVodka, 0, 0, SRCCOPY);
			//			break;
			//		case BLOCK_O_GLUE:
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitOGlue, 0, 0, SRCCOPY);
			//			break;
			//		}

			//		if (gameInfo.boardGame[l][c] > 1999) {
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitSnakeEnemy, 0, 0, SRCCOPY);
			//		}
			//		if (gameInfo.boardGame[l][c] == myId) {
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitSnake, 0, 0, SRCCOPY);
			//		}
			//		if (gameInfo.boardGame[l][c] == myId2) {
			//			BitBlt(memdc, x, y, x + 20, y + 20, hbitSnake2, 0, 0, SRCCOPY);
			//		}

			//		x += 20;
			//	}
			//	x = 0;
			//	y += 20;
			//}
			//BitBlt(hdc, 0, 0, 800, 650, memdc, 0, 0, SRCCOPY);

			
			BitBlt(hdc, 0, 0, 800, 650, memdc, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);
			break;
		}
		
		case WM_SIZE:
		{
			break;
		}

		case WM_ERASEBKGND:
			break;
			
		default:
		{
			// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar") // não é efectuado nenhum processamento, apenas se segue o "default" do Windows			
			return(DefWindowProc(hWnd, messg, wParam, lParam));
		}
			
	}
	
	return(0);
}

void editResourceOnPaint(int resource) {
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	TCHAR fileToOpen[20];

	switch (resource) {
		case IDB_SNAKE_YELLOW:
			_tcscpy(fileToOpen, TEXT(" snake__1.bmp"));
			break;
		case IDB_WALL1:
			_tcscpy(fileToOpen, TEXT(" wall__1.bmp"));
			break;
		case IDB_APPLE:
			_tcscpy(fileToOpen, TEXT(" apple__1.bmp"));
			break;
		default:
			break;
	}

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	//GetModuleFileName(TEXT("mspaint.exe"), executavel, 256);

	// Start the child process. 
	if (
		CreateProcess(
			TEXT("C:\\WINDOWS\\system32\\mspaint.exe"),		// No module name (use command line)
			fileToOpen,										// Command line
			NULL,											// Process handle not inheritable
			NULL,											// Thread handle not inheritable
			0,												// Set handle inheritance to FALSE
			0,												// No creation flags
			NULL,											// Use parent's environment block
			NULL,											// Use parent's starting directory 
			&si,											// Pointer to STARTUPINFO structure
			&pi)											// Pointer to PROCESS_INFORMATION structure
		)
	{
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		_stprintf_s(error, 1024, TEXT("Error... (%d)\n"), GetLastError());
		createErrorMessageBox(error);
	}
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
	data dataGame;

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

//USER is type --> REMOTE
void startRemote() {

	while (1) {

		/*_tcscpy(lpszPipeRemoteName, TEXT("\\\\"));
		_tcscat(lpszPipeRemoteName, domain);
		_tcscat(lpszPipeRemoteName, lpszPipename);

		log = ImpersonateLoggedOnUser(hUserToken);*/

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
			_stprintf_s(error, 1024, TEXT("Create file error and not BUSY...(%d)\n"), GetLastError());
			createErrorMessageBox(error);
			return -1;
		}

		if (!WaitNamedPipe(lpszPipename, 30000)) {
			_stprintf_s(error, 1024, TEXT("Waited 30 seconds, exit..\n"));
			createErrorMessageBox(error);
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
		_stprintf_s(error, 1024, TEXT("SetNamedPipeHandleState failed... (%d)"), GetLastError());
		createErrorMessageBox(error);
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
		_stprintf_s(error, 1024, TEXT("Creating thread... (%d)"), GetLastError());
		createErrorMessageBox(error);
		return -1;
	}
}

//SEND COMMANDS THROUGH SHAREDMEMORY
void sendCommand(data newData) {
	HANDLE eWriteToServerNP;	// HANDLE para o evento leitura
	OVERLAPPED overLapped = { 0 };

	void(*setDataSHM)(data);

	newData.playerId = myId;
	newData.playerId2 = myId2;
	

	if (typeClient == LOCALCLIENT) {
		// GET DLL FUNCTION - setSHM
		setDataSHM = (void(*)(data)) GetProcAddress(hSnakeDll, "setDataSHM");
		if (setDataSHM == NULL) {
			_stprintf_s(error, 1024, TEXT("[SHM ERROR] GetProcAddress - setDataSHM()"), GetLastError());
			createErrorMessageBox(error);
			return;
		}
		// CALL DLL FUNCTION
		setDataSHM(newData);
		// CALL EVENT FOR SERVER READ SHARED MEMORY
		SetEvent(eWriteToServerSHM);
	}
	else {
		eWriteToServerNP = CreateEvent(
			NULL,		// default security
			TRUE,		// manual reset
			FALSE,		// not signaled
			NULL);		// nao precisa de nome, interno ao processo

		if (eWriteToServerNP == NULL) {
			_stprintf_s(error, 1024, TEXT("Create Event failed... (%d)"), GetLastError());
			createErrorMessageBox(error);
			return -1;
		}

		// WRITE NAMED PIPES
		ZeroMemory(&overLapped, sizeof(overLapped));
		ResetEvent(eWriteToServerNP);
		overLapped.hEvent = eWriteToServerNP;

		fSucess = WriteFile(
			hPipe,				// pipe handle
			&newData,		// message
			DataStructSize,		// message size
			&cbWritten,			// ptr to save number of written bytes
			&overLapped);		// not nul -> not overlapped I/O

		WaitForSingleObject(eWriteToServerNP, INFINITE);

		GetOverlappedResult(hPipe, &overLapped, &cbWritten, FALSE); // no wait
		if (cbWritten < DataStructSize) {
			_stprintf_s(error, 1024, TEXT("Write File failed... (%d)"), GetLastError());
			createErrorMessageBox(error);
		}

	} // END ELSE

	
}


//----------------------------------------------------
// DRAW BITMAPS
//----------------------------------------------------
void bitmap(left, right, top, bot, hbit) {
	hdc = GetDC(hWnd);
	HDC auxmemdc = CreateCompatibleDC(hdc);

	SelectObject(auxmemdc, hbit);
	//BitBlt(hdc, left, top, right, bot, auxmemdc, 0, 0, SRCCOPY);
	ReleaseDC(hWnd, hdc);
	BitBlt(memdc, left, top, right, bot, auxmemdc, 0, 0, SRCCOPY);
	DeleteDC(auxmemdc);

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
		
		gameInfo = getInfoSHM();
		// Receive Command from server
		manageCommandsReceived(gameInfo);

		ResetEvent(eReadFromServerSHM);

	}

	return 1;
}

//THREAD RESPONSABLE FOR REMOTE USERS
DWORD WINAPI ThreadClientReader(LPVOID PARAMS) {
	data fromServer;

	DWORD cbBytesRead = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)PARAMS;	// a info enviada é o handle

	HANDLE eReadFromServerNP;
	OVERLAPPED overLapped = { 0 };

	if (hPipe == NULL) {
		_stprintf_s(error, 1024, TEXT("Thread Reader - handle its null..."));
		createErrorMessageBox(error);
		return -1;
	}

	eReadFromServerNP = CreateEvent(
		NULL,		// defalt security
		TRUE,		// resetEvent -> requisito do Overlapped I/O
		FALSE,		// not signaled
		NULL);		// name not needed

	if (eReadFromServerNP == NULL) {
		_stprintf_s(error, 1024, TEXT("Client - read event cannot be created..."));
		createErrorMessageBox(error);
		return -1;
	}

	readerAlive = 1;
	//_tprintf(TEXT("Thread Reader - receiving messages..."));

	while (mayContinue) {

		// prepare ReadReady event
		ZeroMemory(&overLapped, sizeof(overLapped));
		overLapped.hEvent = eReadFromServerNP;
		ResetEvent(eReadFromServerNP);

		fSuccess = ReadFile(
			hPipe,			// pipe handle (params)
			&gameInfo,	// read bytes buffer
			GameInfoStructSize,		// msg size
			&cbBytesRead,	// msg size to read
			&overLapped);	// not null -> not overlapped

		WaitForSingleObject(eReadFromServerNP, INFINITE);
		//_tprintf(TEXT("\nRead success..."));

		if (!fSuccess || cbBytesRead < GameInfoStructSize) {
			GetOverlappedResult(hPipe, &overLapped, &cbBytesRead, FALSE); // No wait
			if (cbBytesRead < DataStructSize) {
				_tprintf(TEXT("[ERROR] Read file failed... (%d)"), GetLastError());
			}
		}

		// Receive Command from server
		manageCommandsReceived(gameInfo);

		

	}

	readerAlive = 0;
	//_tprintf(TEXT("Thread Reader ending\n"));
	return 1;
}

void manageCommandsReceived(GameInfo gameInfo) {
	
	if (gameInfo.playerId == 1000 || gameInfo.playerId == myId || gameInfo.playerId == myId2) {
		switch (gameInfo.commandId) {
			case GAME_OVER:
				running = FALSE;
				createMessageBox(TEXT("Game over.."));
				break;
			case REFRESH_BOARD:
			{
				hdc = GetDC(hWnd);
				int x = 0, y = 0;
				for (int l = 0; l < gameInfo.nRows; l++) {
					for (int c = 0; c < gameInfo.nColumns; c++) {

						switch (gameInfo.boardGame[l][c]) {
						case BLOCK_EMPTY:
							bitmap(x, x + 20, y, y + 20, hbitGround);
							break;
						case BLOCK_FOOD:
							bitmap(x, x + 20, y, y + 20, hbitApple);
							break;
						case BLOCK_WALL:
							bitmap(x, x + 20, y, y + 20, hbitwall);
							break;
						case BLOCK_ICE:
							bitmap(x, x + 20, y, y + 20, hbitIce);
							break;
						case BLOCK_GRANADE:
							bitmap(x, x + 20, y, y + 20, hbitGranade);
							break;
						case BLOCK_OIL:
							bitmap(x, x + 20, y, y + 20, hbitOil);
							break;
						case BLOCK_VODKA:
							bitmap(x, x + 20, y, y + 20, hbitVodka);
							break;
						case BLOCK_GLUE:
							bitmap(x, x + 20, y, y + 20, hbitGlue);
							break;
						case BLOCK_O_OIL:
							bitmap(x, x + 20, y, y + 20, hbitOOil);
							break;
						case BLOCK_O_VODKA:
							bitmap(x, x + 20, y, y + 20, hbitOVodka);
							break;
						case BLOCK_O_GLUE:
							bitmap(x, x + 20, y, y + 20, hbitOGlue);
							break;
						}

						if (gameInfo.boardGame[l][c] > 1999) {
							bitmap(x, x + 20, y, y + 20, hbitSnakeEnemy);
						}
						if (gameInfo.boardGame[l][c] == myId) {
							bitmap(x, x + 20, y, y + 20, hbitSnake);
						}
						if (gameInfo.boardGame[l][c] == myId2) {
							bitmap(x, x + 20, y, y + 20, hbitSnake2);
						}

						x += 20;
					}
					x = 0;
					y += 20;
				}
				ReleaseDC(hWnd, hdc);
				InvalidateRect(NULL, NULL, TRUE);
				break;
			}
			case JOIN_GAME:
				DialogBox(hThisInst, (LPCSTR)IDD_DIALOG_JOIN_GAME, hWnd, (DLGPROC)DialogJoinGame);
				break;
			case ERROR_CANNOT_CREATE_GAME:
				cannotCreate = TRUE;
				break;
			case ERROR_CANNOT_JOIN_GAME:
				createErrorMessageBox(TEXT("Cannot join game.."));
				break;
			default:
				break;
			}	

	}

	
}