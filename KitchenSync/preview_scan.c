#include "kitchen_sync.h"

static void displayErrorBox(LPTSTR);
static int previewFolderPairSource(HWND, struct PairNode **, struct Project *);
static int previewFolderPairTarget(HWND, struct PairNode **, struct Project *);
static int listTreeContent(struct PairNode **, wchar_t *, wchar_t *);
static int listForRemoval(struct PairNode **, wchar_t *);
DWORD CALLBACK entryPointSource(LPVOID);
DWORD CALLBACK entryPointTarget(LPVOID);

static void displayErrorBox(LPTSTR lpszFunction)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	lpDisplayBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen((LPCTSTR)lpMsgBuf) + (size_t)lstrlen((LPCTSTR)lpszFunction) + (size_t)40) * sizeof(TCHAR));
	if (lpDisplayBuf)
	{
		swprintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %lu: %s"), lpszFunction, dw, (LPTSTR)lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
		HeapFree(GetProcessHeap(), 0, lpMsgBuf);
		HeapFree(GetProcessHeap(), 0, lpDisplayBuf);
	}
}

// list the folder names within the supplied folder level only to the specified listbox
int listSubFolders(HWND hwnd, wchar_t *folder)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(folder) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, folder, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("listSubFolders"));
		return dwError;
	}

	int position = 0;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0)
				continue;

#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"LSF() Dir: %s", ffd.cFileName);
	writeFileW(LOG_FILE, buf);
#endif
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listSubFolders"));

	FindClose(hFind);
	return dwError;
}

// find each matching folder pair under this project and send it for Preview
void previewProject(HWND hwnd, struct ProjectNode **head_ref, struct PairNode **pairs, wchar_t *projectName)
{
	struct ProjectNode *current = *head_ref;
	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			struct Project project = { 0 };
			wcscpy_s(project.name, MAX_LINE, projectName);
			wcscpy_s(project.pair.source, MAX_LINE, current->project.pair.source);
			wcscpy_s(project.pair.destination, MAX_LINE, current->project.pair.destination);

			previewFolderPair(hwnd, pairs, &project);
		}
		current = current->next;
	} while (*head_ref != NULL && current != NULL);
}

// send one specific folder pair for Preview in both directions
void previewFolderPair(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	//previewFolderPairSource(hwnd, pairs, project);

	// reverse source & destination
	struct Project reversed = { 0 };
	wcscpy_s(reversed.name, MAX_LINE, project->name);
	wcscpy_s(reversed.pair.source, MAX_LINE, project->pair.destination);
	wcscpy_s(reversed.pair.destination, MAX_LINE, project->pair.source);

	//previewFolderPairTarget(hwnd, pairs, &reversed);

	HANDLE threads[2];
	DWORD threadIDs[2];

	struct Arguments sourceArgs = { hwnd, pairs, project };

	threads[0] = CreateThread(NULL, 0, entryPointSource, &sourceArgs, 0, &threadIDs[0]);
	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create source->destination thread");
		writeFileW(LOG_FILE, buf);
		return;
	}

	struct Arguments targetArgs = { hwnd, pairs, &reversed };

	threads[1] = CreateThread(NULL, 0, entryPointTarget, &targetArgs, 0, &threadIDs[1]);
	if (threads[1] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create destination->source thread");
		writeFileW(LOG_FILE, buf);
		return;
	}

	WaitForMultipleObjects(2, threads, TRUE, INFINITE);
	sortPairNodes(pairs);
}

DWORD CALLBACK entryPointSource(LPVOID arguments)
{
	struct Arguments *args = (struct Arguments *)arguments;
	HWND hwnd = args->hwnd;
	struct PairNode **pairs = args->pairs;
	struct Project *project = args->project;

	previewFolderPairSource(hwnd, pairs, project);
	return 0;
}

DWORD CALLBACK entryPointTarget(LPVOID arguments)
{
	struct Arguments *args = (struct Arguments *)arguments;
	HWND hwnd = args->hwnd;
	struct PairNode **pairs = args->pairs;
	struct Project *project = args->project;

	previewFolderPairTarget(hwnd, pairs, project);
	return 0;
}

// this recursively adds all the files/folders in and below the supplied folders to the pairs list
static int listTreeContent(struct PairNode **pairs, wchar_t *source, wchar_t *destination)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(source) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, source, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("listTreeContent"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		// combine source path \ new filename
		wchar_t currentItem[MAX_LINE] = { 0 };
		if (wcslen(source) + wcslen(ffd.cFileName) + 1 > MAX_LINE)
		{
			writeFileW(LOG_FILE, L"Folder path is full, can't add filename");
			MessageBox(NULL, L"Folder path is full, can't add filename", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		catPath(currentItem, source, ffd.cFileName);

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// recursive folder reading
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t subFolder[MAX_LINE] = { 0 };
			wcscpy_s(subFolder, MAX_LINE, currentItem);

			if (wcslen(subFolder) >= MAX_LINE)
			{
				writeFileW(LOG_FILE, L"Sub-folder path is full, can't add \\");
				MessageBox(NULL, L"Sub-folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
				return 0;
			}
			wcscat(subFolder, L"\\");
			listTreeContent(pairs, currentItem, destination);
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"listTreeContent() Dir: %s", currentItem);
	writeFileW(LOG_FILE, buf);
#endif
		}
		else
		{
			// add to files list
			addPair(pairs, currentItem, destination, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listTreeContent"));

	FindClose(hFind);
	return dwError;
}

// this recursively adds all the files/folders in and below the supplied folder to the pairs list for removal
int listForRemoval(struct PairNode **pairs, wchar_t *path)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(path) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, path, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("listForRemoval"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		// combine path \ new filename
		wchar_t currentItem[MAX_LINE] = { 0 };
		if (wcslen(path) + wcslen(ffd.cFileName) + 1 > MAX_LINE)
		{
			writeFileW(LOG_FILE, L"Folder path is full, can't add filename");
			MessageBox(NULL, L"Folder path is full, can't add filename", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		catPath(currentItem, path, ffd.cFileName);

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// recursive folder reading
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t subFolder[MAX_LINE] = { 0 };
			wcscpy_s(subFolder, MAX_LINE, currentItem);

			if (wcslen(subFolder) >= MAX_LINE)
			{
				writeFileW(LOG_FILE, L"Sub-folder path is full, can't add \\");
				MessageBox(NULL, L"Sub-folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
				return 0;
			}
			wcscat(subFolder, L"\\");
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"listForRemoval() Dir: %s", currentItem);
	writeFileW(LOG_FILE, buf);
#endif
			listForRemoval(pairs, currentItem);
		}
		else
		{
			// add to files list
			addPair(pairs, L"Delete file", currentItem, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listForRemoval"));

	FindClose(hFind);
	return dwError;
}

//FIX some files are passed without path so are duplicated in the list
static int previewFolderPairSource(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, project->pair.source, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t newSource[MAX_LINE];
		catPath(newSource, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE] = { 0 };
		catPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists
			// if exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project subFolder = { 0 };
				wcscpy_s(subFolder.name, MAX_LINE, project->name);
				wcscpy_s(subFolder.pair.source, MAX_LINE, newSource);
				wcscpy_s(subFolder.pair.destination, MAX_LINE, destination);

#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairSource() for %s -> %s", newSource, destination);
	writeFileW(LOG_FILE, buf);
#endif
				previewFolderPairSource(hwnd, pairs, &subFolder);
			}
			else // target folder does not exist
			{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"dump entire tree with listTreeContent() for %s -> %s", newSource, destination);
	writeFileW(LOG_FILE, buf);
#endif
				// if new, add entire folder contents to list (recursive but without comparison)
				listTreeContent(pairs, newSource, destination);
			}
		}
		else // item is a file
		{
			// check if target file exists
			if (fileExists(destination))
			{
#if DEV_MODE
{
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file exists, compare date/time and sizes");
	writeFileW(LOG_FILE, buf);
}
#endif
				LONGLONG targetSize = getFileSize(destination);

				// if different size, add file to list
				if (targetSize != filesize.QuadPart)
				{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file is out of date, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
					addPair(pairs, newSource, destination, filesize.QuadPart);
				}
				else // size is the same so test date/time
				{
					// if last write time is different between source and destination, add file to list
					if (fileDateIsDifferent(ffd.ftCreationTime, ffd.ftLastAccessTime, ffd.ftLastWriteTime, destination))
					{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"source & target files have different date/time, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
						addPair(pairs, newSource, destination, filesize.QuadPart);
					}
				}
			}
			else // destination file does not exist
			{
				// add to files list
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file does not exist, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
				addPair(pairs, newSource, destination, filesize.QuadPart);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairSource"));

	FindClose(hFind);
	return dwError;
}

// reversed folder direction
static int previewFolderPairTarget(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, project->pair.source, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t newSource[MAX_LINE];
		catPath(newSource, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE] = { 0 };
		catPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists
			// if exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project subFolder = { 0 };
				wcscpy_s(subFolder.name, MAX_LINE, project->name);
				wcscpy_s(subFolder.pair.source, MAX_LINE, newSource);
				wcscpy_s(subFolder.pair.destination, MAX_LINE, destination);

#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairTarget() for %s -> %s", newSource, destination);
	writeFileW(LOG_FILE, buf);
#endif
				previewFolderPairTarget(hwnd, pairs, &subFolder);
			}
			else // target folder does not exist
			{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"Entire folder tree to be removed %s", newSource);
	writeFileW(LOG_FILE, buf);
#endif
				addPair(pairs, L"Delete folder", newSource, filesize.QuadPart);
				listForRemoval(pairs, newSource);
			}
		}
		else // item is a file
		{
			// check if target file exists
			if (!fileExists(destination))
//			{
//				//TODO do I care about this?
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = { 0 };
//	swprintf(buf, MAX_LINE, L"target file exists, does it matter?");
//	writeFileW(LOG_FILE, buf);
//#endif
//			}
//			else // destination file does not exist
			{
				// remove source file
#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file does not exist, add source %s for deletion", newSource);
	writeFileW(LOG_FILE, buf);
#endif
				addPair(pairs, L"Delete file", newSource, filesize.QuadPart);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairTarget"));

	FindClose(hFind);
	return dwError;
}