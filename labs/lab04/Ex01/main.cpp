#define UNICODE
#define _UNICODE

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>

#define GEN_FILES
#undef  DEBUG 
#define BUF_SIZE 256

typedef struct thread_s {
	DWORD id;
	TCHAR filename[BUF_SIZE];
	DWORD size;
	DWORD32* array;
} thread_t;

void GenerateFiles(DWORD nFiles, DWORD n, LPTSTR filename);
void GenerateFile(DWORD n, LPTSTR filename);
void SortAndMerge(DWORD n, LPTSTR filename, LPTSTR outputFilename);

DWORD WINAPI Sort(LPVOID);

int DWORD32_compare(void const* a, void const* b);
void Merge(DWORD32* arr1, DWORD len1, DWORD32* arr2, DWORD len2, DWORD32* res);
void WriteBinaryArray(LPTSTR filename, DWORD32 size, DWORD32* arr);


int _tmain(int argc, LPTSTR argv[])
{
	if (argc != 4) {
		_tprintf(_T("USAGE: n filename\n"));
		return 1;
	}

#ifdef GEN_FILES
	srand(42);

	DWORD n = _ttoi(argv[1]);
	LPTSTR filename = argv[2];
	GenerateFiles(n, 1024, filename);
#endif

	LPTSTR outputFilename = argv[3];
	SortAndMerge(n, filename, outputFilename);
}

/// <summary>
/// Generate input files.
/// </summary>
/// <param name="nFiles">Number of input files to generate</param>
/// <param name="n">Maximum size of each file (in terms of DWORD32 values)</param>
/// <param name="filename">Base filename</param>
void GenerateFiles(DWORD nFiles, DWORD n, LPTSTR filename) {
	TCHAR buffer[BUF_SIZE];

	while (nFiles--) {
		DWORD size = (rand() % (n / 2)) + (n / 2);

		_stprintf(buffer, _T("%s-%d.bin"), filename, nFiles);
		GenerateFile(size, buffer);
	}
}

/// <summary>
/// Generate an input file.
/// </summary>
/// <param name="n">Maximum size of each file (in terms of DWORD32 values)</param>
/// <param name="filename">Filename</param>
void GenerateFile(DWORD n, LPTSTR filename) {
	HANDLE fH;
	DWORD nW;

	fH = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fH == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Cannot open input file. Error: %x\n"), GetLastError());
		exit(2);
	}

	DWORD32 value = n;

	WriteFile(fH, &value, sizeof(DWORD32), &nW, NULL);
	if (nW != sizeof(DWORD32)) {
		_tprintf(_T("Error: %lu\n"), GetLastError());
	}

	for (int i = 0; i < n; i++) {
		value = rand();
		WriteFile(fH, &value, sizeof(DWORD32), &nW, NULL);
		if (nW != sizeof(DWORD32)) {
			_tprintf(_T("Error: %lu\n"), GetLastError());
		}
	}

	CloseHandle(fH);
}

/// <summary>
/// Sort the input files and merge them.
/// </summary>
/// <param name="n">Number of input files</param>
/// <param name="filename">Base filename</param>
/// <param name="outputFilename">Output filename</param>
void SortAndMerge(DWORD n, LPTSTR filename, LPTSTR outputFilename) {
	TCHAR buffer[BUF_SIZE];

	DWORD current_size = 0;
	DWORD32* current_array = NULL;

	HANDLE* threadHandles;
	thread_t* params;

	// allocate stuff
	threadHandles = (HANDLE*)malloc(n * sizeof(HANDLE));
	params = (thread_t*)malloc(n * sizeof(thread_t));

	for (int i = 0; i < n; i++) {
		_stprintf(params[i].filename, _T("%s-%d.bin"), filename, i);
		threadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Sort, &params[i], 0, &params[i].id);
	}

	for (int i = 0; i < n; i++) {
		DWORD ret = WaitForMultipleObjects(n - i, threadHandles + i, FALSE, INFINITE);
		DWORD index = ret - WAIT_OBJECT_0;

		// generate a new array
		DWORD32* new_array = (DWORD32*)malloc((current_size + params[i + index].size) * sizeof(DWORD32));
		// and merge with current array
		Merge(current_array, current_size, params[i + index].array, params[i + index].size, new_array);
		// update current size
		current_size += params[i + index].size;

		// replace current array with the new one and free some memory
		free(params[i + index].array);
		free(current_array);
		current_array = new_array;

		// close the handle of the terminated thread
		CloseHandle(threadHandles[i + index]);

		params[i + index] = params[i];
		threadHandles[i + index] = threadHandles[i];
	}

	WriteBinaryArray(outputFilename, current_size, current_array);

	free(current_array);

	free(threadHandles);
	free(params);
}

/// <summary>
/// Main function of the worker threads.
/// </summary>
/// <param name="lpParam">Void pointer to the thread's parameters.</param>
/// <returns>Return value of the thread</returns>
DWORD WINAPI Sort(LPVOID lpParam) {
	thread_t* data = (thread_t*)lpParam;

	HANDLE fH = CreateFile(data->filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fH == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Cannot open input file. Error: %x\n"), GetLastError());
		ExitThread(2);
	}

	DWORD nRead;
	if (!ReadFile(fH, &data->size, sizeof(DWORD32), &nRead, NULL)) {
		_tprintf(_T("Cannot read input file. Error: %x\n"), GetLastError());
		ExitThread(3);
	}

	data->array = (DWORD32*)malloc(data->size * sizeof(DWORD32));

	for (int i = 0; i < data->size; i++) {
		if (!ReadFile(fH, data->array + i, sizeof(DWORD32), &nRead, NULL)) {
			_tprintf(_T("Cannot read input file. Error: %x\n"), GetLastError());
			ExitThread(3);
		}
	}

	qsort(data->array, data->size, sizeof(DWORD32), &DWORD32_compare);

	CloseHandle(fH);

	return 0;
}

/// <summary>
/// Merge two DWORD32 arrays.
/// </summary>
/// <param name="arr1">First array</param>
/// <param name="len1">Length of the first array</param>
/// <param name="arr2">Second array</param>
/// <param name="len2">Length of the second array</param>
/// <param name="res">Result array</param>
void Merge(DWORD32* arr1, DWORD len1, DWORD32* arr2, DWORD len2, DWORD32* res) {
	DWORD i = 0;
	DWORD k = 0;

	while (i < len1 && k < len2) {
		if (arr1[i] < arr2[k])
			res[i + k] = arr1[i++];
		else
			res[i + k] = arr2[k++];
	}

	while (i < len1) res[i + k] = arr1[i++];
	while (k < len2) res[i + k] = arr2[k++];
}

/// <summary>
/// Compare two DWORD32 values.
/// </summary>
int DWORD32_compare(void const* a, void const* b) {
	return (*(DWORD32*)a - *(DWORD32*)b);
}

/// <summary>
/// Write an array of DWORD32 values in binary mode.
/// </summary>
/// <param name="filename">Output filename</param>
/// <param name="size">Size of the array to write</param>
/// <param name="arr">Array to write</param>
void WriteBinaryArray(LPTSTR filename, DWORD32 size, DWORD32* arr) {
	HANDLE fH;
	DWORD nW;

	fH = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fH == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Cannot open input file. Error: %x\n"), GetLastError());
		exit(2);
	}

	WriteFile(fH, &size, sizeof(DWORD32), &nW, NULL);
	if (nW != sizeof(DWORD32)) {
		_tprintf(_T("Error: %lu\n"), GetLastError());
	}

	for (int i = 0; i < size; i++) {
		WriteFile(fH, arr + i, sizeof(DWORD32), &nW, NULL);
		if (nW != sizeof(DWORD32)) {
			_tprintf(_T("Error: %lu\n"), GetLastError());
		}
	}

	CloseHandle(fH);
}