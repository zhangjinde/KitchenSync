#include "kitchen_sync.h"

void shutDown(HWND hwnd, struct ProjectNode **head_ref)
{
	writeSettings(INI_FILE, hwnd);
	saveProjects(PROJECTS, head_ref);
	PostQuitMessage(0);
}

void centerWindow(HWND hwnd)
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

void clearArray(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

void clearArrayW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

void clearNewlines(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

void clearNewlinesW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

void writeFile(char *filename, char *text)
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

void writeFileW(char *filename, wchar_t *text)
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

void writeSettings(char *filename, HWND hwnd)
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

void readSettings(char *filename, HWND hwnd)
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

void loadProjects(char *filename, struct ProjectNode **head_ref, HWND hwnd)
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
	sortProjectNodes(head_ref);
	fillListbox(head_ref, hwnd);
}

void fillListbox(struct ProjectNode **head_ref, HWND hwnd)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"fillListbox()");
#endif

	// populate listbox
	wchar_t *currentProjectName = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!currentProjectName)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for currentProjectName from projects file");
		return;
	}

	int position = 0;
	struct ProjectNode *current = *head_ref;

	while (current != NULL)
	{
		// add folder pair, into a new or existing project section
		if (wcscmp(currentProjectName, current->project.name) != 0) // new project section
		{
			// add blank line between each project
			if (position > 0)
				SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)"");

			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)current->project.name);
		}

		wchar_t *buffer = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buffer)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for folder pair buffer from projects file");
			return;
		}

		swprintf(buffer, MAX_LINE * 4, L"%s -> %s", current->project.pair.source, current->project.pair.destination);
		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)buffer);
		wcscpy_s(currentProjectName, MAX_LINE, current->project.name);
		current = current->next;
	}
}

void saveProjects(char *filename, struct ProjectNode **head_ref)
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

	struct ProjectNode *current = *head_ref;
	int count = 0;

	while (current != NULL)
	{
		wchar_t *buf = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buf)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for buf for projects file");
			return;
		}

		// don't save project with no pairs
		if (wcslen(current->project.pair.source) > 0 && wcslen(current->project.pair.destination) > 0)
		{
			swprintf(buf, MAX_LINE * 4, L"%s,%s,%s", current->project.name, current->project.pair.source, current->project.pair.destination);
			writeFileW(PROJECTS, buf);
		}

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
bool isProjectName(wchar_t *text, int len)
{
	for (int i = 0; i < len && text[i] != '\0'; ++i)
		if (text[i] == '>')
			return false;
	return true;
}

// look backwards to find project name from selected pair
void findProjectName(HWND hwnd, LRESULT selectedRow, wchar_t *selectedRowText, wchar_t *projectName)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"findProjectName()");
#endif

	LRESULT projectPosition = selectedRow;
	bool found = false;

	do
	{
		int textLen = (int)SendMessage(hwnd, LB_GETTEXT, --projectPosition, (LPARAM)selectedRowText);
		if (isProjectName(selectedRowText, textLen))
		{
			wcscpy_s(projectName, MAX_LINE, selectedRowText);
			found = true;
		}
	}
	while (!found && projectPosition >= 0);
}

// reload source & destination folder pair listboxes
void reloadFolderPairs(struct ProjectNode *projectsHead, wchar_t *projectName, HWND src, HWND dst)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"reloadFolderPairs()");
#endif

	int position = 0;
	struct ProjectNode *current = projectsHead;

	while (current != NULL)
	{
		// add all folder pairs from current project
		if (wcscmp(projectName, current->project.name) == 0)
		{
			SendMessage(src, LB_ADDSTRING, position, (LPARAM)current->project.pair.source);
			SendMessage(dst, LB_ADDSTRING, position++, (LPARAM)current->project.pair.destination);
		}
		current = current->next;
	}
}