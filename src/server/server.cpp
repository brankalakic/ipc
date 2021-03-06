#include <iostream>
#include <exception>
#include <map>
#include <ctime>
#include <mutex>

#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include "../common/utils.h"
#include "BaseClass.h"
#include "Car.h"

using namespace std;

std::mutex mtxDoubleStorage;
std::mutex mtxStrStorage;

#define BUFSIZE 512

DWORD WINAPI InstanceThread(LPVOID);

struct InstanceData {
	HANDLE handle;
	std::map<double, double>* doubleStorage;
	std::map<double, TCHAR*>* strStorage;
};

void sendResult(unsigned char ** buf_result, size_t  &buf_result_len, LPVOID result, int type, HANDLE hPipe) {
	size_t result_len = 0;

	switch (type) {
	case INTEGER:
		buf_result_len = 2 * sizeof(int);
		*buf_result = new unsigned char[buf_result_len];
		memcpy(*buf_result, &type, sizeof(int));
		memcpy(*buf_result + sizeof(int), result, sizeof(int));
		break;
	case TCHAR_P:
		if (result == NULL) {
			result_len = 0;
		}
		else {
			result_len = (_tcslen((TCHAR*)result) + 1) * sizeof(TCHAR);
		}
		buf_result_len = sizeof(int) + sizeof(size_t) + result_len;
		*buf_result = new unsigned char[buf_result_len];
		memcpy(*buf_result, &type, sizeof(int));
		memcpy(*buf_result + sizeof(int), &result_len, sizeof(size_t));
		if (result != NULL) {
			memcpy(*buf_result + sizeof(int) + sizeof(size_t), result, result_len);
		}
		break;
	case VOID_T:
		buf_result_len = sizeof(int);
		*buf_result = new unsigned char[buf_result_len];
		memcpy(*buf_result, &type, sizeof(int));
		break;
	default:
		buf_result_len = sizeof(int);
		*buf_result = new unsigned char[buf_result_len];
		memcpy(*buf_result, &type, sizeof(int));
		printf("Type not supported.\n");
		break;

	}

	BOOL fSuccess;
	DWORD cbWritten;
	fSuccess = WriteFile(hPipe, *buf_result, buf_result_len, &cbWritten, NULL);

	if (!fSuccess)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
	}
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread receiving and executing commands from the client.
// Note this allows the main loop to continue executing, potentially creating 
// more threads of of this procedure to run concurrently, depending on the 
// number of incoming client connections.
{
	HANDLE hHeap = GetProcessHeap();

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	bool fSuccess = false;
	HANDLE hPipe = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf("   InstanceThread exitting.\n");
		return -1;
	}

	hPipe = ((InstanceData*)lpvParam)->handle;
	int commandCode;
	TCHAR * message = NULL;

	size_t messageLength;
	size_t clsIdLength;
	TCHAR* clsId;

	// reading the overhead

	unsigned char * buf = new unsigned char[sizeof(int) + sizeof(size_t)];
	fSuccess = ReadFile(hPipe, buf, sizeof(int) + sizeof(size_t), &cbBytesRead, NULL);

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
	}
	else 
	{
		commandCode = *(int*)(buf);
		messageLength = *(size_t*)(buf + sizeof(int));
		delete [] buf;

		buf = new unsigned char[messageLength - sizeof(int) - sizeof(size_t)];

		fSuccess = ReadFile(hPipe, buf, messageLength - sizeof(int) - sizeof(size_t), &cbBytesRead, NULL); 

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

		}
		else
		{
			if (commandCode == INSTANTIATE_OBJECT) {
				clsIdLength = *(size_t*)buf;
				clsId = (TCHAR*)(buf + sizeof(size_t));

				BaseClass* object = BaseClass::classRegistry[std::wstring(clsId)]->clone();
				int id = object->getId();
				BaseClass::objectRegistry[id] = object;

				fSuccess = WriteFile(hPipe, &id, sizeof(int), &cbWritten, NULL);
				if (!fSuccess)
				{
					_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
				}

			}

			if (commandCode == GET_OBJECT_ATTRIBUTE) {
				unsigned char * buf_result = NULL;
				int objectId = *(int*)buf;
				size_t idLength = *(size_t*)(buf + sizeof(int));
				std::wstring attributeId = std::wstring((TCHAR*)(buf + sizeof(size_t) + sizeof(int)));

				BaseClass *object = BaseClass::objectRegistry[objectId];
				LPVOID result;
				int type = INVALID_TYPE;
				result = object->reflectAttribute(attributeId, &type);

				size_t buf_result_len=0;
				size_t result_len = 0;
				sendResult(&buf_result, buf_result_len, result, type, hPipe);

				if (buf_result != NULL) {
					delete[] buf_result;
				}
			}

			if (commandCode == RUN_METHOD) {
				unsigned char * buf_result = NULL;
				int objectId = *(int*)buf;
				size_t idLength = *(size_t*)(buf + sizeof(int));
				std::wstring methodId = std::wstring((TCHAR*)(buf + sizeof(size_t) + sizeof(int)));

				BaseClass *object = BaseClass::objectRegistry[objectId];

				LPVOID result;
				int type = INVALID_TYPE;
				size_t argc = 0;
				TCHAR* argv[MAX_ARGS];
				size_t offset = sizeof(size_t) + sizeof(int) + idLength;
				argc = *(size_t*)(buf + offset);
				offset += sizeof(size_t);
				for (int i = 0; i < argc; i++) {
					size_t leni = *(size_t*)(buf + offset);
					argv[i] = new TCHAR[leni];
					memcpy(argv[i], buf + offset + sizeof(leni), leni);
				}

				result = object->reflectMethod(methodId, argc, argv, &type);

				size_t buf_result_len;
				buf_result = NULL;
				sendResult(&buf_result, buf_result_len, result, type, hPipe);

				if (buf_result != NULL) {
					delete[] buf_result;
				}

				if (argc > 0) {
					for (int i = 0; i < argc; i++) {
						delete[] argv[i];
					}
				}
			}

			if (commandCode == SEND_DOUBLE) {
				double key = *((double*)buf);
				double val = *(double*)(buf + sizeof(double));
				mtxDoubleStorage.lock();
				(*((InstanceData*)lpvParam)->doubleStorage)[key] = val;
				mtxDoubleStorage.unlock();
				bool done = true;
				writeBooleanToPipe(hPipe, done);
			}

			if (commandCode == GET_DOUBLE) {
				double key = *(double*)buf;
				double val;
				unsigned char *buf;
				size_t len = sizeof(bool);

				bool exists = false;
				mtxDoubleStorage.lock();
				if (((InstanceData*)lpvParam)->doubleStorage->count(key) > 0) {
					val = (*((InstanceData*)lpvParam)->doubleStorage)[key];
					len += sizeof(double);
					exists = true;
				}
				mtxDoubleStorage.unlock();

				buf = new unsigned char[len];
				memcpy(buf, &exists, sizeof(bool));
				if (exists) {
					memcpy(buf+sizeof(bool), &val, sizeof(double));
				}
					
				sendBuffer(buf, len, hPipe);
				delete[] buf;
			}

			if (commandCode == SEND_STRING) {
				double key = *(double*)buf;
				size_t strLength = *(size_t*)(buf + sizeof(double));

				TCHAR * message = (TCHAR*)HeapAlloc(hHeap, 0, strLength * sizeof(TCHAR));
				_tcscpy_s(message, strLength / 2, (TCHAR*)(buf + sizeof(double) + sizeof(size_t)));

				mtxStrStorage.lock();
				(*((InstanceData*)lpvParam)->strStorage)[key] = message;
				mtxStrStorage.unlock();
				bool done = true;
				writeBooleanToPipe(hPipe, done);
			}

			if (commandCode == GET_STRING) {
				double key = *(double*)buf;
				TCHAR* message=NULL;
				unsigned char *buf;
				size_t len = sizeof(bool);

				bool exists = false;
				size_t msgLen;
				mtxStrStorage.lock();
				if (((InstanceData*)lpvParam)->strStorage->count(key) > 0) {
					message = (*((InstanceData*)lpvParam)->strStorage)[key];
					len += sizeof(size_t);
					msgLen = (wcslen(message) + 1) * sizeof(TCHAR);
					len += msgLen;
					exists = true;
				}
				mtxStrStorage.unlock();

				buf = new unsigned char[len];
				memcpy(buf, &exists, sizeof(bool));
				if (exists) {
					memcpy(buf + sizeof(bool), &msgLen, sizeof(size_t));
					memcpy(buf + sizeof(bool) + sizeof(size_t), message, msgLen);
				}

				sendBuffer(buf, len, hPipe);
				delete[] buf;
			}
		}

		if (buf != NULL) {
			delete[] buf;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	delete (InstanceData*)lpvParam;
	printf("InstanceThread exitting.\n");
	return 0;
}


int _tmain(VOID)
{
	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	const TCHAR * pipeName = TEXT("\\\\.\\pipe\\mycpipe");

	// storage for numbers and strings with keys
	std::map<double, double> doubleStorage;
	std::map<double, TCHAR*> strStorage;

	// registering class Car
	Car * car = new Car();
	BaseClass::registerClass(std::wstring(L"Car"), (BaseClass*)car);


	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle the request from 
	// that client, and this loop is free to wait for the
	// next command. It is an infinite loop.

	InstanceData* instanceData;

	for (;;)
	{
		_tprintf(TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), pipeName);
		hPipe = CreateNamedPipe(pipeName, 
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
			PIPE_UNLIMITED_INSTANCES, 
			BUFSIZE,
			BUFSIZE,
			0,
			NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
			return -1;
		}

		fConnected = ConnectNamedPipe(hPipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		instanceData = new InstanceData();
		instanceData->handle = hPipe;
		instanceData->doubleStorage = &doubleStorage;
		instanceData->strStorage = &strStorage;

		if (fConnected)
		{
			printf("Client command connected, creating a processing thread.\n");

			// Create a thread for this client command. 
			hThread = CreateThread(NULL, 0, InstanceThread, (LPVOID)instanceData, 0, &dwThreadId); 

			if (hThread == NULL)
			{
				_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
				return -1;
			}
			else
			{
				CloseHandle(hThread);
			}
		}
		else
		{
			// The client could not connect, so close the pipe. 
			CloseHandle(hPipe);
		}
	}

	map<double, TCHAR*>::iterator it;

	HANDLE hHeap = GetProcessHeap();
	for (it = strStorage.begin(); it != strStorage.end(); it++)
	{
		HeapFree(hHeap, 0, it->second);
	}

	return 0;
}
