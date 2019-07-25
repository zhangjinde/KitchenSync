#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define DEV_MODE 1

#define LOG_FILE "kitchen_sync.log"
#define INI_FILE "kitchen_sync.ini"
#define PROJECTS "kitchen_sync.prj"
#define MAX_LINE 512
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 1024
#define WINDOW_HEIGHT_MINIMUM 200
#define ID_LISTBOX_PROJECTS 20
#define ID_LISTBOX_PAIRS_SOURCE 21
#define ID_LISTBOX_PAIRS_DEST 22
#define ID_LISTBOX_SYNC 23
#define ID_LISTBOX_ADD_SOURCE 24
#define ID_LISTBOX_ADD_DEST 25
#define ID_BUTTON_PREVIEW 30
#define ID_BUTTON_SYNC 31
#define ID_BUTTON_ADD_PROJECT 32
#define ID_BUTTON_ADD_FOLDER_PAIR 33
#define ID_BUTTON_ADD_PAIR 34
#define ID_BUTTON_PAIR_ADD 35
#define ID_BUTTON_OK 36
#define ID_BUTTON_CANCEL 37
#define ID_BUTTON_DELETE 38
#define ID_LABEL_ADD_PROJECT_NAME 40
#define ID_LABEL_PAIR_SOURCE 41
#define ID_LABEL_PAIR_DEST 42
#define ID_EDIT_ADD_PROJECT_NAME 43
#define ID_EDIT_PAIR_SOURCE 44
#define ID_EDIT_PAIR_DEST 45
#define ID_PROGRESS_BAR 50
#define ID_TIMER1 51

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if DEV_MODE
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define assert(expression)
#endif

#include <stdio.h>
//#include <stdlib.h> // rand()
//#include <time.h> // time()
//#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include "kitchen_sync.h"
#pragma comment(lib, "comctl32.lib")

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
};

struct PairNode
{
	struct Pair pair;
	struct PairNode *next;
};

struct Project
{
	wchar_t name[MAX_LINE];
	struct Pair pair;
};

struct ProjectNode
{
	struct Project project;
	struct ProjectNode *next;
};

// create empty lists
static struct PairNode *filesHead = NULL;
static struct ProjectNode *projectsHead = NULL;

static LRESULT CALLBACK mainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customListboxProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK projectNameWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customProjectNameProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK folderPairWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customFolderPairProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customSourceListboxProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customDestinationListboxProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC originalListboxProc;
static WNDPROC originalProjectNameProc;
static WNDPROC originalFolderPairProc;
static WNDPROC originalSourceListboxProc;
static WNDPROC originalDestinationListboxProc;
static HWND mainHwnd;
static HWND tabHwnd;
static HWND projectNameHwnd;
static HWND folderPairHwnd;
static HWND lbProjectsHwnd;
static HWND lbPairsHwnd;
static HWND lbSourceHwnd;
static HWND lbDestHwnd;
static HWND lbPairSourceHwnd;
static HWND lbPairDestHwnd;
static HWND bProjectNameOK;
static HINSTANCE instance;

static void shutDown(void);
static void centerWindow(HWND);
static void clearArray(char *, int);
static void clearArrayW(wchar_t *, int);
static void clearNewlines(char *, int);
static void clearNewlinesW(wchar_t *, int);
static void writeFile(char *, char *);
static void writeFileW(char *, wchar_t *);
static void writeSettings(char *, HWND);
static void readSettings(char *, HWND);
static void loadProjects(char *, struct ProjectNode **);
static void saveProjects(char *, struct ProjectNode **);
static void appendPairNode(struct PairNode **, struct Pair);
static void appendProjectNode(struct ProjectNode **, struct Project);
static void deletePairNode(struct PairNode **, int);
static void deleteProjectNode(struct ProjectNode **, int);
static void deletePairList(struct PairNode **);
static void deleteProjectList(struct ProjectNode **);
static void getProjectName(void);
static void addFolderPair(void);
static void fillListbox(struct ProjectNode **);
static void sortProjectNodes(struct ProjectNode **);
static void DisplayErrorBox(LPTSTR);
static int countPairNodes(struct PairNode *);
static int countProjectNodes(struct ProjectNode *);
static int listDir(HWND, wchar_t *);
static bool isProjectName(wchar_t *, int);

static bool showingFolderPair = false;
static bool showingProjectName = false;
static wchar_t projectName[MAX_LINE] = {0};
static wchar_t folderPair[MAX_LINE] = {0};
static wchar_t sourceFolder[MAX_LINE] = {0};
static wchar_t destFolder[MAX_LINE] = {0};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	//srand((unsigned int)time(NULL));
	instance = hInstance;

	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = mainWndProc;
	wc.lpszClassName = L"KitchenSync";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	mainHwnd = CreateWindowEx(WS_EX_LEFT,
		wc.lpszClassName,
		L"KitchenSync v0.1",
		//WS_OVERLAPPEDWINDOW,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (!mainHwnd)
	{
		MessageBox(NULL, L"Window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// reset log file
	FILE *lf = fopen(LOG_FILE, "wt, ccs=UNICODE");
	if (lf == NULL)
		MessageBox(NULL, L"Can't open log file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	else
	{
		fwprintf(lf, L"%s\n", L"Log file reset");
		fclose(lf);
	}

	ShowWindow(mainHwnd, nCmdShow);
	readSettings(INI_FILE, mainHwnd);
	loadProjects(PROJECTS, &projectsHead);

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static INITCOMMONCONTROLSEX icex = {0};
	static HWND bPreview, bSync, bAddProject, bAddFolders, bAddPair, bDelete;
	static bool listboxClicked = false;

	switch (msg)
	{
		case WM_CREATE:
			// initialize common controls
			icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			icex.dwICC = ICC_TAB_CLASSES;
			InitCommonControlsEx(&icex);

			RECT rc = {0};
			GetClientRect(hwnd, &rc);

			tabHwnd = CreateWindow(WC_TABCONTROL, L"",
			WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
			5, 5, rc.right-10, rc.bottom-10, hwnd, NULL, instance, NULL);

			if (tabHwnd == NULL)
			{
				writeFileW(LOG_FILE, L"Failed to create tab control");
				break;
			}

			TCITEM tie = {0};
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = L"Projects";
			TabCtrl_InsertItem(tabHwnd, 0, &tie);
			tie.pszText = L"Folder Pairs";
			TabCtrl_InsertItem(tabHwnd, 1, &tie);
			tie.pszText = L"Sync";
			TabCtrl_InsertItem(tabHwnd, 2, &tie);
			tie.pszText = L"Settings";
			TabCtrl_InsertItem(tabHwnd, 3, &tie);

		// projects

			// listbox
			lbProjectsHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				10, 80, rc.right-20, rc.bottom-90, hwnd, (HMENU)ID_LISTBOX_PROJECTS, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbProjectsHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			bAddProject = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Project",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				10, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_ADD_PROJECT, NULL, NULL);

			bAddFolders = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Folder Pair",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				140, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_ADD_FOLDER_PAIR, NULL, NULL);

			bDelete = CreateWindowEx(WS_EX_LEFT, L"Button", L"Delete",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				270, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_DELETE, NULL, NULL);

			bPreview = CreateWindowEx(WS_EX_LEFT, L"Button", L"Preview",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				400, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_PREVIEW, NULL, NULL);

		// folder pairs

			// source listbox
			lbSourceHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				10, 80, (rc.right / 2)-20, rc.bottom-90, hwnd, (HMENU)ID_LISTBOX_PAIRS_SOURCE, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbSourceHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			// destination listbox
			lbDestHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				(rc.right / 2)+5, 80, (rc.right / 2)-20, rc.bottom-90, hwnd, (HMENU)ID_LISTBOX_PAIRS_DEST, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbDestHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			bAddPair = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Pair",
				WS_CHILD | WS_TABSTOP,
				10, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_ADD_PAIR, NULL, NULL);

		// sync

			// listbox
			lbPairsHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				10, 80, rc.right-20, rc.bottom-110, hwnd, (HMENU)ID_LISTBOX_SYNC, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbPairsHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			bSync = CreateWindowEx(WS_EX_LEFT, L"Button", L"Sync",
				WS_CHILD | WS_TABSTOP | WS_DISABLED,
				10, 40, 120, 30, hwnd, (HMENU)ID_BUTTON_SYNC, NULL, NULL);

			break;
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case TCN_SELCHANGING:
				{
					// save any page status here before the change
					return false;
				}
				case TCN_SELCHANGE:
				{
					int page = TabCtrl_GetCurSel(tabHwnd);
					switch (page)
					{
						case 0: // projects
						{
							clearArrayW(projectName, MAX_LINE);
							clearArrayW(folderPair, MAX_LINE);
							SendMessage(lbSourceHwnd, LB_RESETCONTENT, 0, 0);
							SendMessage(lbDestHwnd, LB_RESETCONTENT, 0, 0);
							ShowWindow(lbProjectsHwnd, SW_SHOW);
							ShowWindow(lbPairsHwnd, SW_HIDE);
							ShowWindow(lbSourceHwnd, SW_HIDE);
							ShowWindow(lbDestHwnd, SW_HIDE);
							ShowWindow(bAddProject, SW_SHOW);
							ShowWindow(bAddFolders, SW_SHOW);
							ShowWindow(bAddPair, SW_HIDE);
							ShowWindow(bPreview, SW_SHOW);
							ShowWindow(bSync, SW_HIDE);
							ShowWindow(bDelete, SW_SHOW);
							break;
						}
						case 1: // add folders
						{
							ShowWindow(lbProjectsHwnd, SW_HIDE);
							ShowWindow(lbPairsHwnd, SW_HIDE);
							ShowWindow(lbSourceHwnd, SW_SHOW);
							ShowWindow(lbDestHwnd, SW_SHOW);
							ShowWindow(bAddProject, SW_HIDE);
							ShowWindow(bAddFolders, SW_HIDE);
							ShowWindow(bAddPair, SW_SHOW);
							ShowWindow(bPreview, SW_HIDE);
							ShowWindow(bSync, SW_HIDE);
							ShowWindow(bDelete, SW_HIDE);
							break;
						}
						case 2: // sync
						{
							ShowWindow(lbProjectsHwnd, SW_HIDE);
							ShowWindow(lbPairsHwnd, SW_SHOW);
							ShowWindow(lbSourceHwnd, SW_HIDE);
							ShowWindow(lbDestHwnd, SW_HIDE);
							ShowWindow(bAddProject, SW_HIDE);
							ShowWindow(bAddFolders, SW_HIDE);
							ShowWindow(bAddPair, SW_HIDE);
							ShowWindow(bPreview, SW_HIDE);
							ShowWindow(bSync, SW_SHOW);
							ShowWindow(bDelete, SW_SHOW);
							break;
						}
						case 3: // settings
						{
							ShowWindow(lbProjectsHwnd, SW_HIDE);
							ShowWindow(lbPairsHwnd, SW_HIDE);
							ShowWindow(lbSourceHwnd, SW_HIDE);
							ShowWindow(lbDestHwnd, SW_HIDE);
							ShowWindow(bAddProject, SW_HIDE);
							ShowWindow(bAddFolders, SW_HIDE);
							ShowWindow(bAddPair, SW_HIDE);
							ShowWindow(bPreview, SW_HIDE);
							ShowWindow(bSync, SW_HIDE);
							ShowWindow(bDelete, SW_HIDE);
							break;
						}
					}
				}
			}
		}
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_LISTBOX_PROJECTS)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					EnableWindow(bAddFolders, FALSE);
					EnableWindow(bPreview, FALSE);
					EnableWindow(bDelete, FALSE);

					// get row index
					LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							EnableWindow(bPreview, TRUE);
							EnableWindow(bDelete, TRUE);

							if (isProjectName(selectedRowText, textLen))
								EnableWindow(bAddFolders, TRUE);
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							if (isProjectName(selectedRowText, textLen))
							{
								wcscpy_s(projectName, MAX_LINE, selectedRowText);
								getProjectName();
							}
							else
							{
								wcscpy_s(folderPair, MAX_LINE, selectedRowText);

								// look backwards to find project name
								LRESULT projectPosition = selectedRow;
								bool found = false;
								do
								{
									textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, --projectPosition, (LPARAM)selectedRowText);
									if (isProjectName(selectedRowText, textLen))
									{
										wcscpy_s(projectName, MAX_LINE, selectedRowText);
										found = true;
									}
								}
								while (!found && projectPosition >= 0);

								// change to folder pair tab and populate listboxes, then edit selected pair
								SendMessage(tabHwnd, TCM_SETCURFOCUS, 1, 0);
								int position = 0;
								struct ProjectNode *current = projectsHead;

								while (current != NULL)
								{
									// add all folder pairs from current project
									if (wcscmp(projectName, current->project.name) == 0)
									{
										SendMessage(lbSourceHwnd, LB_ADDSTRING, position, (LPARAM)current->project.pair.source);
										SendMessage(lbDestHwnd, LB_ADDSTRING, position++, (LPARAM)current->project.pair.destination);
									}
									current = current->next;
								}

								addFolderPair();
							}
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_LISTBOX_PAIRS_SOURCE)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbSourceHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO store selected row in case of Del key?
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbSourceHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO edit folder pair
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_LISTBOX_PAIRS_DEST)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbDestHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO store selected row in case of Del key?
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbDestHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO edit folder pair
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_LISTBOX_SYNC)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairsHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO store selected row in case of Del key?
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairsHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO edit file pair
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_BUTTON_ADD_PROJECT)
			{
				getProjectName();
			}

			if (LOWORD(wParam) == ID_BUTTON_ADD_FOLDER_PAIR)
			{
#if DEV_MODE
				writeFileW(LOG_FILE, L"add folder pair");
#endif
				// get row index
				LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0 && isProjectName(selectedRowText, textLen))
					{
						wcscpy_s(projectName, MAX_LINE, selectedRowText);
#if DEV_MODE
						wchar_t buf[100] = {0};
						swprintf(buf, 100, L"Selected project name: %s", projectName);
						writeFileW(LOG_FILE, buf);
#endif
						// change to folder pair tab and populate listboxes
						SendMessage(tabHwnd, TCM_SETCURFOCUS, 1, 0);
						int position = 0;
						struct ProjectNode *current = projectsHead;

						while (current != NULL)
						{
							// add all folder pairs from current project
							if (wcscmp(projectName, current->project.name) == 0)
							{
								SendMessage(lbSourceHwnd, LB_ADDSTRING, position, (LPARAM)current->project.pair.source);
								SendMessage(lbDestHwnd, LB_ADDSTRING, position++, (LPARAM)current->project.pair.destination);
							}
							current = current->next;
						}

						addFolderPair();
					}
				}
			}

			if (LOWORD(wParam) == ID_BUTTON_ADD_PAIR)
			{
#if DEV_MODE
				writeFileW(LOG_FILE, L"add pair");
#endif
				addFolderPair();
			}

			break;
		case WM_SIZE:
		{
			//RECT rc = {0};
			//GetWindowRect(hwnd, &rc);
			//int windowHeight = rc.bottom - rc.top;

			// resize listbox & textbox to fit window
			//SetWindowPos(listboxProjectsHwnd, HWND_TOP, 10, 10, 500, windowHeight - 320, SWP_SHOWWINDOW);
			//SetWindowPos(listboxPairsHwnd, HWND_TOP, 520, 10, WINDOW_WIDTH-500-45, 25, SWP_SHOWWINDOW);

			// maintain main window width
			//SetWindowPos(hwnd, HWND_TOP, rc.left, rc.top, WINDOW_WIDTH, windowHeight, SWP_SHOWWINDOW);

			// force minimum height
			//if (windowHeight < WINDOW_HEIGHT_MINIMUM)
			//	SetWindowPos(hwnd, HWND_TOP, rc.left, rc.top, WINDOW_WIDTH, WINDOW_HEIGHT_MINIMUM, SWP_SHOWWINDOW);
		}
			break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					shutDown();
					break;
			}
			break;
		case WM_DESTROY:
			shutDown();
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK customListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_LBUTTONDBLCLK:
		//{
		//	listboxDoubleClicked = true;
		//	writeFileW(LOG_FILE, L"Listbox double clicked");
		//}
		//	break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					shutDown();
					break;
			}
			break;
	}
	return CallWindowProc(originalListboxProc, hwnd, msg, wParam, lParam);
}

static void getProjectName(void)
{
	static WNDCLASSEX wcProjectName = {0};
	static bool projectNameClassRegistered = false;

	if (showingProjectName)
		return;
	showingProjectName = true;

	if (!projectNameClassRegistered)
	{
		wcProjectName.cbSize = sizeof(WNDCLASSEX);
		wcProjectName.cbClsExtra = 0;
		wcProjectName.cbWndExtra = 0;
		wcProjectName.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcProjectName.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcProjectName.hIcon = NULL;
		wcProjectName.hIconSm = NULL;
		wcProjectName.hInstance = instance;
		wcProjectName.lpfnWndProc = projectNameWndProc;
		wcProjectName.lpszClassName = L"getProjectName";
		wcProjectName.lpszMenuName = NULL;
		wcProjectName.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcProjectName))
		{
			MessageBox(NULL, L"Get Project Name window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			projectNameClassRegistered = true;
	}

	projectNameHwnd = CreateWindowEx(WS_EX_LEFT,
		wcProjectName.lpszClassName,
		L"Get Project Name",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		435, 130,
		NULL, NULL,
		instance, NULL);

	if (!projectNameHwnd)
	{
		MessageBox(NULL, L"Get Project Name window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(projectNameHwnd, SW_SHOW);
}

static LRESULT CALLBACK projectNameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bProjectNameCancel, lProjectName, eProjectName;

	switch (msg)
	{
		case WM_CREATE:
			SetTimer(hwnd, ID_TIMER1, 100, NULL);

			lProjectName = CreateWindowEx(WS_EX_LEFT, L"Static", L"Enter project name",
				WS_VISIBLE | WS_CHILD,
				10, 15, 150, 25, hwnd, (HMENU)ID_LABEL_ADD_PROJECT_NAME, NULL, NULL);

			eProjectName = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
				140, 10, 170, 25, hwnd, (HMENU)ID_EDIT_ADD_PROJECT_NAME, NULL, NULL);
			originalProjectNameProc = (WNDPROC)SetWindowLongPtr(eProjectName, GWLP_WNDPROC, (LONG_PTR)customProjectNameProc);

			bProjectNameOK = CreateWindowEx(WS_EX_LEFT, L"Button", L"OK",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 10, 80, 25, hwnd, (HMENU)ID_BUTTON_OK, NULL, NULL);

			bProjectNameCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 45, 80, 25, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

			SendMessage(eProjectName, EM_LIMITTEXT, MAX_LINE, 0);
			if (wcslen(projectName) > 0)
				SetWindowText(eProjectName, projectName);
			centerWindow(hwnd);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_BUTTON_OK)
			{
				wchar_t newProjectName[MAX_LINE];
				GetWindowText(eProjectName, newProjectName, MAX_LINE);

				// blank project name
				if (wcslen(newProjectName) == 0)
				{
					writeFileW(LOG_FILE, L"Blank name used");
					DestroyWindow(hwnd);
					break;
				}

#if DEV_MODE
				wchar_t buf[100] = {0};
				swprintf(buf, 100, L"New project name: %s", newProjectName);
				writeFileW(LOG_FILE, buf);
#endif

				// new project being added
				if (wcslen(newProjectName) > 0 && wcslen(projectName) == 0)
				{
					// check if pre-existing name used
					bool existing = false;
					struct ProjectNode *node = projectsHead;
					while (node != NULL)
					{
						if (wcscmp(node->project.name, newProjectName) == 0)
						{
							writeFileW(LOG_FILE, L"Pre-existing name used");
							existing = true;
							break;
						}

						node = node->next;
					}

					if (!existing)
					{
						writeFileW(LOG_FILE, L"New project added");
						struct Project project = {0};
						wcscpy_s(project.name, MAX_LINE, newProjectName);
						wcscpy_s(project.pair.source, MAX_LINE, L"source");
						wcscpy_s(project.pair.destination, MAX_LINE, L"destination");

						//TODO how to handle the folder pair when adding a new project?
						// store project name and change tab to Add Folders
						// replace the "source" & "destination" pair or don't create them at all?

						appendProjectNode(&projectsHead, project);
					}
				}

				// existing project being renamed
				if (wcslen(newProjectName) > 0 && wcslen(projectName) > 0)
				{
					// replace all occurrences of old project name with new name
					struct ProjectNode *node = projectsHead;
					while (node != NULL)
					{
						if (wcscmp(node->project.name, projectName) == 0)
							wcscpy_s(node->project.name, MAX_LINE, newProjectName);

						node = node->next;
					}
				}

				SendMessage(lbProjectsHwnd, LB_RESETCONTENT, 0, 0);
				sortProjectNodes(&projectsHead);
				fillListbox(&projectsHead);
				DestroyWindow(hwnd);
			}

			if (LOWORD(wParam) == ID_BUTTON_CANCEL)
			{
				DestroyWindow(hwnd);
			}
			break;
		case WM_TIMER:
			if (wParam == ID_TIMER1)
			{
				SetFocus(eProjectName);
				KillTimer(hwnd, ID_TIMER1);
			}
			break;
		case WM_DESTROY:
			showingProjectName = false;
			clearArrayW(projectName, MAX_LINE);
			DestroyWindow(hwnd);
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customProjectNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(projectNameHwnd);
					break;
				case VK_RETURN:
					SendMessage(bProjectNameOK, BM_CLICK, 0, 0);
					break;
				case 'A': // CTRL A
					if (GetAsyncKeyState(VK_CONTROL))
						SendMessage(hwnd, EM_SETSEL, 0, -1);
					break;
			}
			break;
	}

	return CallWindowProc(originalProjectNameProc, hwnd, msg, wParam, lParam);
}

static void addFolderPair(void)
{
	static WNDCLASSEX wcFolderPair = {0};
	static bool folderPairClassRegistered = false;

	if (showingFolderPair)
		return;
	showingFolderPair = true;

	if (!folderPairClassRegistered)
	{
		wcFolderPair.cbSize = sizeof(WNDCLASSEX);
		wcFolderPair.cbClsExtra = 0;
		wcFolderPair.cbWndExtra = 0;
		wcFolderPair.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcFolderPair.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcFolderPair.hIcon = NULL;
		wcFolderPair.hIconSm = NULL;
		wcFolderPair.hInstance = instance;
		wcFolderPair.lpfnWndProc = folderPairWndProc;
		wcFolderPair.lpszClassName = L"addFolderPair";
		wcFolderPair.lpszMenuName = NULL;
		wcFolderPair.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcFolderPair))
		{
			MessageBox(NULL, L"Add Folder Pair window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			folderPairClassRegistered = true;
	}

	folderPairHwnd = CreateWindowEx(WS_EX_LEFT,
		wcFolderPair.lpszClassName,
		L"Add Folder Pair",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		NULL, NULL,
		instance, NULL);

	if (!folderPairHwnd)
	{
		MessageBox(NULL, L"Add Folder Pair window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(folderPairHwnd, SW_SHOW);
}

static LRESULT CALLBACK folderPairWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bFolderPairAdd, bFolderPairCancel, lSource, lDestination, eSource, eDestination;

	switch (msg)
	{
		case WM_CREATE:
			lSource = CreateWindowEx(WS_EX_LEFT, L"Static", L"Source",
				WS_VISIBLE | WS_CHILD,
				10, 10, 80, 25, hwnd, (HMENU)ID_LABEL_PAIR_SOURCE, NULL, NULL);

			eSource = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
				10, 30, 370, 25, hwnd, (HMENU)ID_EDIT_PAIR_SOURCE, NULL, NULL);
			originalFolderPairProc = (WNDPROC)SetWindowLongPtr(eSource, GWLP_WNDPROC, (LONG_PTR)customFolderPairProc);

			lbPairSourceHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				10, 60, 370, 450, hwnd, (HMENU)ID_LISTBOX_ADD_SOURCE, NULL, NULL);
			originalSourceListboxProc = (WNDPROC)SetWindowLongPtr(lbPairSourceHwnd, GWLP_WNDPROC, (LONG_PTR)customSourceListboxProc);

			lDestination = CreateWindowEx(WS_EX_LEFT, L"Static", L"Destination",
				WS_VISIBLE | WS_CHILD,
				400, 10, 80, 25, hwnd, (HMENU)ID_LABEL_PAIR_DEST, NULL, NULL);

			eDestination = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
				400, 30, 370, 25, hwnd, (HMENU)ID_EDIT_PAIR_DEST, NULL, NULL);
			originalFolderPairProc = (WNDPROC)SetWindowLongPtr(eDestination, GWLP_WNDPROC, (LONG_PTR)customFolderPairProc);

			lbPairDestHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				400, 60, 370, 450, hwnd, (HMENU)ID_LISTBOX_ADD_DEST, NULL, NULL);
			originalDestinationListboxProc = (WNDPROC)SetWindowLongPtr(lbPairDestHwnd, GWLP_WNDPROC, (LONG_PTR)customDestinationListboxProc);

			bFolderPairAdd = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				290, 520, 80, 25, hwnd, (HMENU)ID_BUTTON_PAIR_ADD, NULL, NULL);

			bFolderPairCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				410, 520, 80, 25, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

			SendMessage(eSource, EM_LIMITTEXT, MAX_LINE, 0);
			SendMessage(eDestination, EM_LIMITTEXT, MAX_LINE, 0);
			centerWindow(hwnd);

			//TODO load folder contents based on selected pair
			wcscpy_s(sourceFolder, MAX_LINE, L"C:\\KitchenTest");
			wcscpy_s(destFolder, MAX_LINE, L"C:\\Games");

			SetWindowText(eSource, sourceFolder);
			SetWindowText(eDestination, destFolder);
			listDir(lbPairSourceHwnd, sourceFolder);
			listDir(lbPairDestHwnd, destFolder);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_LISTBOX_ADD_SOURCE)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairSourceHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO store selected row in case of Add
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairSourceHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO change to selected folder and load contents
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							wchar_t currentFolder[MAX_LINE] = {0};
							wcscpy_s(currentFolder, MAX_LINE, sourceFolder);

							if (wcscmp(selectedRowText, L"..") == 0)
							{
								size_t len = wcslen(sourceFolder);
								while (sourceFolder[len] != '\\' && len > 0)
									--len;

								sourceFolder[len] = '\0';
							}
							else
							{
								GetWindowText(eSource, sourceFolder, MAX_LINE);
								wcscat(sourceFolder, L"\\");
								wcscat(sourceFolder, selectedRowText);
							}

							SendMessage(lbPairSourceHwnd, LB_RESETCONTENT, 0, 0);
							if (listDir(lbPairSourceHwnd, sourceFolder))
								SetWindowText(eSource, sourceFolder);
							else
							{
								//TODO handle errors properly
								// if error, redisplay folder contents
								SendMessage(lbPairSourceHwnd, LB_RESETCONTENT, 0, 0);
								listDir(lbPairSourceHwnd, currentFolder);
							}
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_LISTBOX_ADD_DEST)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairDestHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO store selected row in case of Add
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							//TODO
						}
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairDestHwnd, LB_GETCURSEL, 0, 0);
					if (selectedRow != LB_ERR)
					{
						//TODO change to selected folder and load contents
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbPairDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
						{
							wchar_t currentFolder[MAX_LINE] = {0};
							wcscpy_s(currentFolder, MAX_LINE, destFolder);

							if (wcscmp(selectedRowText, L"..") == 0)
							{
								size_t len = wcslen(destFolder);
								while (destFolder[len] != '\\' && len > 0)
									--len;

								destFolder[len] = '\0';
							}
							else
							{
								GetWindowText(eDestination, destFolder, MAX_LINE);
								wcscat(destFolder, L"\\");
								wcscat(destFolder, selectedRowText);
							}

							SendMessage(lbPairDestHwnd, LB_RESETCONTENT, 0, 0);
							if (listDir(lbPairDestHwnd, destFolder))
								SetWindowText(eDestination, destFolder);
							else
							{
								// if error, redisplay folder contents
								SendMessage(lbPairDestHwnd, LB_RESETCONTENT, 0, 0);
								listDir(lbPairDestHwnd, currentFolder);
							}
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_BUTTON_PAIR_ADD)
			{
				//TODO add new pair
				DestroyWindow(hwnd);
			}

			if (LOWORD(wParam) == ID_BUTTON_CANCEL)
			{
				DestroyWindow(hwnd);
			}
			break;
		case WM_DESTROY:
			showingFolderPair = false;
			DestroyWindow(hwnd);
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customFolderPairProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(folderPairHwnd);
					break;
				case VK_RETURN:
					//TODO change to directory
					break;
				case 'A': // CTRL A
					if (GetAsyncKeyState(VK_CONTROL))
						SendMessage(hwnd, EM_SETSEL, 0, -1);
					break;
			}
			break;
	}

	return CallWindowProc(originalFolderPairProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK customSourceListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_LBUTTONDBLCLK:
		//{
		//	writeFileW(LOG_FILE, L"Listbox double clicked");
		//}
		//	break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(folderPairHwnd);
					break;
			}
			break;
	}
	return CallWindowProc(originalSourceListboxProc, hwnd, msg, wParam, lParam);
}

//TODO this can be handled by source proc?
LRESULT CALLBACK customDestinationListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_LBUTTONDBLCLK:
		//{
		//	writeFileW(LOG_FILE, L"Listbox double clicked");
		//}
		//	break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(folderPairHwnd);
					break;
			}
			break;
	}
	return CallWindowProc(originalDestinationListboxProc, hwnd, msg, wParam, lParam);
}

static void shutDown(void)
{
	writeSettings(INI_FILE, mainHwnd);
	saveProjects(PROJECTS, &projectsHead);
	PostQuitMessage(0);
}

static void centerWindow(HWND hwnd)
{
	RECT rc = {0};

	GetWindowRect(hwnd, &rc);
	int windowWidth = rc.right - rc.left;
	int windowHeight = rc.bottom - rc.top;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, (screenWidth - windowWidth) / 2,
		(screenHeight - windowHeight) / 2, 0, 0, SWP_NOSIZE);
}

static void clearArray(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

static void clearArrayW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

static void clearNewlines(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

static void clearNewlinesW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

static void writeFile(char *filename, char *text)
{
	//while (state.writing)
	//	Sleep(100);

	//state.writing = true;

	FILE *f = fopen(filename, "a");
	if (f == NULL)
	{
		MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	fprintf(f, "%s\n", text);
	fclose(f);
	//state.writing = false;
}

static void writeFileW(char *filename, wchar_t *text)
{
	//while (state.writing)
	//	Sleep(100);

	//state.writing = true;

	FILE *f = fopen(filename, "at, ccs=UNICODE");
	if (f == NULL)
	{
		MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	fwprintf(f, L"%s\n", text);
	fclose(f);
	//state.writing = false;
}

static void writeSettings(char *filename, HWND hwnd)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"writeSettings()");
#endif

	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"Error saving settings!");
		return;
	}

	RECT rc = {0};
	GetWindowRect(hwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowCol = rc.left;
	int windowRow = rc.top;

	fprintf(f, "window_row=%d\n", windowRow);
	fprintf(f, "window_col=%d\n", windowCol);
	fprintf(f, "window_height=%d\n", windowHeight);
	fclose(f);
}

static void readSettings(char *filename, HWND hwnd)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"readSettings()");
#endif

	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		SetWindowPos(hwnd, HWND_TOP, (screenWidth - WINDOW_WIDTH) / 2, (screenHeight - WINDOW_HEIGHT) / 2, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_SHOWWINDOW);
		return;
	}

	int windowHeight = 0;
	int windowCol = 0;
	int windowRow = 0;
	char *line = (char *)malloc(MAX_LINE);
	if (!line)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for line from settings file");
		fclose(f);
		return;
	}

	while (fgets(line, MAX_LINE, f) != NULL)
	{
		if (line[0] == '#')
			continue;

		char *setting = (char *)calloc(MAX_LINE, sizeof(char));
		if (!setting)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for setting from settings file");
			fclose(f);
			return;
		}
		char *value = (char *)calloc(MAX_LINE, sizeof(char));
		if (!value)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for value from settings file");
			fclose(f);
			return;
		}
		char *l = line;
		char *s = setting;
		char *v = value;

		// read setting
		while (*l && *l != '=')
			*s++ = *l++;
		*s = '\0';

		// read value
		++l;
		while (*l)
			*v++ = *l++;
		*v = '\0';

		if (strcmp(setting, "window_row") == 0)		windowRow = atoi(value);
		if (strcmp(setting, "window_col") == 0)		windowCol = atoi(value);
		if (strcmp(setting, "window_height") == 0)	windowHeight = atoi(value);
	}

	fclose(f);

	if (windowHeight == 0)
		windowHeight = WINDOW_HEIGHT;
	if (windowRow == 0)
	{
	 	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		windowRow = (screenHeight - windowHeight) / 2;
	}
	if (windowCol == 0)
	{
	 	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		windowCol = (screenWidth - WINDOW_WIDTH) / 2;
	}

	SetWindowPos(hwnd, HWND_TOP, windowCol, windowRow, WINDOW_WIDTH, windowHeight, SWP_SHOWWINDOW);
}

static void loadProjects(char *filename, struct ProjectNode **head_ref)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"loadProjects()");
#endif

	FILE *f = fopen(filename, "rt, ccs=UNICODE");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"No project history found");
		return;
	}

	wchar_t *line = (wchar_t *)malloc(MAX_LINE * 4);
	if (!line)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for line from projects file");
		fclose(f);
		return;
	}

	while (fgetws(line, MAX_LINE * 2, f) != NULL)
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		wchar_t *name = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!name)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for name from projects file");
			fclose(f);
			return;
		}
		wchar_t *source = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!source)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for source from projects file");
			fclose(f);
			return;
		}
		wchar_t *destination = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!destination)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for destination from projects file");
			fclose(f);
			return;
		}
		wchar_t *l = line;
		wchar_t *n = name;
		wchar_t *s = source;
		wchar_t *d = destination;

		// read name
		while (*l && *l != ',')
			*n++ = *l++;
		*n = '\0';

		// read source
		++l;
		while (*l && *l != ',')
			*s++ = *l++;
		*s = '\0';

		// read destination
		++l;
		while (*l && *l != '\n' && *l != '\0')
			*d++ = *l++;
		*d = '\0';

		wchar_t *buf = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buf)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for logging buf");
			fclose(f);
			return;
		}
		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", name, source, destination);
		writeFileW(LOG_FILE, buf);

		struct Project project = {0};
		wcscpy_s(project.name, MAX_LINE, name);
		wcscpy_s(project.pair.source, MAX_LINE, source);
		wcscpy_s(project.pair.destination, MAX_LINE, destination);

		appendProjectNode(head_ref, project);
	}

#if DEV_MODE
	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Loaded %d projects", countProjectNodes(*head_ref));
	writeFileW(LOG_FILE, buf);
#endif

	fclose(f);

	// add test data
	//struct Project project = {0};
	//wchar_t name[100] = L"name";
	//wchar_t source[100] = L"source";
	//wchar_t destination[100] = L"destination";

	//int rn = rand() % 1024;
	//rn = rand() % 1024;

	//clearArrayW(buf, 100);

	//swprintf(buf, 100, L"%s%d", name, rn);
	//wcscpy_s(project.name, 100, buf);

	//swprintf(buf, 100, L"%s%d", source, rn);
	//wcscpy_s(project.pair.source, 100, buf);

	//swprintf(buf, 100, L"%s%d", destination, rn);
	//wcscpy_s(project.pair.destination, 100, buf);

	//appendProjectNode(head_ref, project);

	sortProjectNodes(head_ref);
	fillListbox(head_ref);
}

static void fillListbox(struct ProjectNode **head_ref)
{
	// populate listbox
	wchar_t *currentProjectName = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!currentProjectName)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for currentProjectName from projects file");
		return;
	}

	int position = 0;
	struct ProjectNode *current = projectsHead;

	while (current != NULL)
	{
		// add folder pair, into a new or existing project section
		if (wcscmp(currentProjectName, current->project.name) != 0) // new project section
		{
			// add blank line between each project
			if (position > 0)
				SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)"");

			SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)current->project.name);
		}

		wchar_t *buffer = (wchar_t *)calloc(MAX_LINE*4, sizeof(wchar_t));
		if (!buffer)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for folder pair buffer from projects file");
			return;
		}

		swprintf(buffer, MAX_LINE*4, L"%s -> %s", current->project.pair.source, current->project.pair.destination);
		SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)buffer);

		wcscpy_s(currentProjectName, MAX_LINE, current->project.name);
		current = current->next;
	}
}

static void saveProjects(char *filename, struct ProjectNode **head_ref)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"saveProjects()");
#endif

	FILE *f = fopen(filename, "wt, ccs=UNICODE");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"Unable to save project history");
		return;
	}

	struct ProjectNode *current = projectsHead;
	int count = 0;

	while (current != NULL)
	{
		wchar_t *buf = (wchar_t *)calloc(MAX_LINE*4, sizeof(wchar_t));
		if (!buf)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for buf for projects file");
			return;
		}

		swprintf(buf, MAX_LINE * 4, L"%s,%s,%s", current->project.name, current->project.pair.source, current->project.pair.destination);
		writeFileW(PROJECTS, buf);

		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", current->project.name, current->project.pair.source, current->project.pair.destination);
		writeFileW(LOG_FILE, buf);

		++count;
		current = current->next;
	}

#if DEV_MODE
	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Saved %d projects", count);
	writeFileW(LOG_FILE, buf);
#endif

	fclose(f);
}

// determine if project name by absence of > character
static bool isProjectName(wchar_t *text, int len)
{
	for (int i = 0; i < len && text[i] != '\0'; ++i)
		if (text[i] == '>')
			return false;
	return true;
}

// append a new node at the end
static void appendPairNode(struct PairNode **head_ref, struct Pair pair)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"appendPairNode()");
#endif

	struct PairNode *newPairNode = (struct PairNode *)malloc(sizeof(struct PairNode));

	if (!newPairNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new pair");
		return;
	}

	if (wcscpy_s(newPairNode->pair.source, MAX_LINE, pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new source");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}
	if (wcscpy_s(newPairNode->pair.destination, MAX_LINE, pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new destination");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}

	newPairNode->next = NULL;

	// if list is empty set newPairNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newPairNode;
		return;
	}

	// divert current last node to newPairNode
	struct PairNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newPairNode;
}

// append a new node at the end
static void appendProjectNode(struct ProjectNode **head_ref, struct Project project)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"appendProjectNode()");
#endif

	struct ProjectNode *newProjectNode = (struct ProjectNode *)malloc(sizeof(struct ProjectNode));

	if (!newProjectNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new project");
		return;
	}

	if (wcscpy_s(newProjectNode->project.name, MAX_LINE, project.name))
	{
		writeFileW(LOG_FILE, L"Failed copying new project name");
		writeFileW(LOG_FILE, project.name);
		free(newProjectNode);
		return;
	}
	if (wcscpy_s(newProjectNode->project.pair.source, MAX_LINE, project.pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new project source");
		writeFileW(LOG_FILE, project.pair.source);
		free(newProjectNode);
		return;
	}
	if (wcscpy_s(newProjectNode->project.pair.destination, MAX_LINE, project.pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new project destination");
		writeFileW(LOG_FILE, project.pair.destination);
		free(newProjectNode);
		return;
	}

	newProjectNode->next = NULL;

	// if list is empty set newProjectNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newProjectNode;
		return;
	}

	// point current last node to newProjectNode
	struct ProjectNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newProjectNode;
}

#if 0
// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deletePairNode(struct PairNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct PairNode *temp = *head_ref;

	// if head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position-1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct PairNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deleteProjectNode(struct ProjectNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct ProjectNode *temp = *head_ref;

	// head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position-1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct ProjectNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// delete the entire linked list
static void deletePairList(struct PairNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct PairNode *current = *head_ref;
	struct PairNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}

// delete the entire linked list
static void deleteProjectList(struct ProjectNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct ProjectNode *current = *head_ref;
	struct ProjectNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}
#endif

// count nodes in list
static int countPairNodes(struct PairNode *head)
{
	int count = 0;
	struct PairNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

// count nodes in list
static int countProjectNodes(struct ProjectNode *head)
{
	int count = 0;
	struct ProjectNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

// sort list
static void sortProjectNodes(struct ProjectNode **head_ref)
{
	if (*head_ref == NULL)
	{
		writeFileW(LOG_FILE, L"Can't sort empty list");
		return;
	}

	struct ProjectNode *head = *head_ref;

	if (head->next == NULL)
	{
		writeFileW(LOG_FILE, L"Can't sort only 1 entry");
		return;
	}

	bool changed;

	do
	{
		changed = false;

		// swap head & second nodes
		if (wcscmp(head->project.name, head->next->project.name) > 0)
		{
			struct ProjectNode *temp = head->next;

			head->next = head->next->next;
			temp->next = head;
			*head_ref = temp;
			head = *head_ref;
			changed = true;
		}

		struct ProjectNode *previous = head;
		struct ProjectNode *current = head->next;

		if (current->next == NULL)
		{
			writeFileW(LOG_FILE, L"Can't sort more, only 2 entries");
			return;
		}

		// swap folder pairs
		while (current != NULL && current->next != NULL)
		{
			if (wcscmp(current->project.name, current->next->project.name) > 0 ||
				(wcscmp(current->project.name, current->next->project.name) == 0 &&
				 wcscmp(current->project.pair.source, current->next->project.pair.source) > 0))
			{
				struct ProjectNode *temp = current->next;

				if (current->next->next != NULL)
					current->next = current->next->next;
				else
					current->next = NULL;
				temp->next = current;
				previous->next = temp;

				changed = true;
			}
			else
				current = current->next;

			previous = previous->next;
		}
	}
	while (changed);
}

static int listDir(HWND hwnd, wchar_t *folder)
{
	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, folder);
	wcscat(szDir, L"\\*");

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	int position = 0;
	//LARGE_INTEGER filesize;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0)
				continue;

			wchar_t buf[MAX_LINE] = {0};
			swprintf(buf, MAX_LINE, L"Dir: %s", ffd.cFileName);
			writeFileW(LOG_FILE, buf);
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
		//else
		//{
		//	filesize.LowPart = ffd.nFileSizeLow;
		//	filesize.HighPart = ffd.nFileSizeHigh;

		//	wchar_t buf[MAX_LINE] = {0};
		//	swprintf(buf, MAX_LINE, L"File: %s, Size: %lld", ffd.cFileName, filesize.QuadPart);
		//	writeFileW(LOG_FILE, buf);
		//	SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		//}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		DisplayErrorBox(TEXT("FindFirstFile"));

	FindClose(hFind);
	return dwError;
}

static void DisplayErrorBox(LPTSTR lpszFunction)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	swprintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, (LPTSTR)lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
