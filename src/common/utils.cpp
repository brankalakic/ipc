#include "utils.h"

bool readBooleanFromPipe(HANDLE hPipe, bool *val) {
	bool fSuccess;
	DWORD cbBytesRead = 0;

	fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		val,    // buffer to receive data 
		sizeof(bool), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if ((!fSuccess || cbBytesRead == 0) && GetLastError() != ERROR_MORE_DATA)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("Broken pipe %d"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return false;
	}

	return true;
}

bool readDoubleFromPipe(HANDLE hPipe, double *val) {
	bool fSuccess;
	DWORD cbBytesRead = 0;

	fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		val,    // buffer to receive data 
		sizeof(double), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbBytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("Broken pipe %d"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return false;
	}

	return true;
}

bool readStringFromPipe(HANDLE hPipe, TCHAR ** message) {
	bool fSuccess;
	size_t msgLength;
	DWORD cbBytesRead = 0;
	HANDLE hHeap = GetProcessHeap();

	fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		&msgLength,    // buffer to receive data 
		sizeof(size_t), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 


	if ((!fSuccess || cbBytesRead == 0) && GetLastError() != ERROR_MORE_DATA)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("Broken pipe, GLE=%d.\n"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return false;
	}

	*message = (wchar_t*)HeapAlloc(hHeap, 0, msgLength/2);


	fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		*message,    // buffer to receive data 
		msgLength, // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbBytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("Broken pipe, GLE=%d.\n"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return false;
	}

	return true;
}

bool writeBooleanToPipe(HANDLE hPipe, bool val) {
	bool fSuccess;
	DWORD cbWritten = 0;

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		&val,                // message 
		sizeof(bool),              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return false;
	}

	return true;
}

BOOL sendBuffer(unsigned char *buf, size_t len, HANDLE hPipe) {

	BOOL fSuccess;
	DWORD cbWritten;
	fSuccess = WriteFile(hPipe,                  // pipe handle 
		buf,                // message 
		len,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess || cbWritten != len)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return fSuccess;
	}
	return fSuccess;
}

bool readIntFromPipe(HANDLE hPipe, int *objectID) {
	DWORD cbBytesRead;
	BOOL fSuccess;

	fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		objectID,    // buffer to receive data 
		sizeof(int), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbBytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("Broken pipe %d"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return FALSE;
	}

	return fSuccess;
}