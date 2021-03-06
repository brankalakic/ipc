#include <windows.h> 
#include <stdio.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <tchar.h>
#include "../common/utils.h"


#define BUFSIZE 512

bool sendString(TCHAR *str, double key, TCHAR *pipename);
bool sendDouble(double val, double key, TCHAR *pipename);
bool getDouble(double key, TCHAR *pipename, double * val);
bool getString(double key, TCHAR *pipename, TCHAR ** str);

bool instantiateClassOnServer(std::wstring classId, int *objectID, TCHAR *pipename);
LPVOID getObjectAttribute(int objectId, std::wstring attributeID, TCHAR *pipename, int *type);
LPVOID runMethodOnServer(int objectID, std::wstring methodID, size_t argc, TCHAR **argv, TCHAR *pipename, int*type);

LPVOID receiveReturnValue(HANDLE hPipe, int* type);

HANDLE connect(wchar_t* pipename) {
	HANDLE hPipe;

	while (1)
	{
		hPipe = CreateFile(
			pipename, 
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING, 
			0, 
			NULL);

	    // Break if the pipe handle is valid. 
		if (hPipe != INVALID_HANDLE_VALUE)
		{
			break;
		}

		DWORD lastError = GetLastError();
		switch (lastError)
		{
		case ERROR_PIPE_BUSY:
			_tprintf(TEXT("Pipe %s is busy, waiting ...\n"), pipename);
			WaitNamedPipe(pipename, 20000);
			break;
		default:
			_tprintf(TEXT("Unable to connect to pipe %s. Will try again in 1 second...\n"), pipename);
			Sleep(1000);
		}
	}

	return hPipe;
}

bool instantiateClassOnServer(std::wstring classId, int *objectID, TCHAR *pipename) {
	BOOL fSuccess;
	size_t classIdLength = (classId.length() + 1) * sizeof(TCHAR);
	size_t len = sizeof(int) + 2 * sizeof(size_t) + classIdLength;
	unsigned char * bytes = new unsigned char[len];
	DWORD cbWritten = 0;
	int cc = INSTANTIATE_OBJECT;

	// connect
	HANDLE hPipe = connect(pipename);

	if (hPipe == NULL) {
		return NULL;
	}

	// make and send request
	memcpy(bytes, &cc, sizeof(int));
	memcpy(bytes + sizeof(int), &len, sizeof(size_t));
	memcpy(bytes + sizeof(int) + sizeof(size_t), &classIdLength, sizeof(size_t));
	memcpy(bytes + sizeof(int) + 2*sizeof(size_t), classId.c_str(), classIdLength);

	fSuccess = sendBuffer(bytes, len, hPipe);

	delete[] bytes;

	if (!fSuccess) {
		return fSuccess;
	}

	// read the id of the instantiated object
	fSuccess = readIntFromPipe(hPipe, objectID);

	CloseHandle(hPipe);
	return fSuccess;
}

LPVOID getObjectAttribute(int objectId, std::wstring attributeID, TCHAR *pipename, int *type) {

	// Connect
	
	HANDLE hPipe = connect(pipename);

	if (hPipe == NULL) {
		return NULL;
	}

	// Make and send request

	size_t attributeIDLength = (attributeID.length() + 1) * sizeof(TCHAR);
	size_t len = 2 * sizeof(int) + 2 * sizeof(size_t) + attributeIDLength;
	unsigned char * buf = new unsigned char[len];
	
	int cc = GET_OBJECT_ATTRIBUTE;
	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(size_t), &objectId, sizeof(int));
	memcpy(buf + 2 * sizeof(int) + sizeof(size_t), &attributeIDLength, sizeof(size_t));
	memcpy(buf + 2 * sizeof(int) + 2 * sizeof(size_t), attributeID.c_str(), attributeIDLength);
	
	BOOL fSuccess = sendBuffer(buf, len, hPipe);

	delete[] buf;

	if (!fSuccess) {
		return NULL;
	}

	// Receive reply

	LPVOID result = receiveReturnValue(hPipe, type);
	
	CloseHandle(hPipe);

	return result;
}

LPVOID runMethodOnServer(int objectID, std::wstring methodID, size_t argc, TCHAR **argv, TCHAR *pipename, int*type) {
	
	// Connect

	HANDLE hPipe = connect(pipename);

	if (hPipe == NULL) {
		return NULL;
	}

	// Make and send request

	size_t methodIDLength = (methodID.length() + 1) * sizeof(TCHAR);
	size_t len = 2 * sizeof(int) + 3 * sizeof(size_t) + methodIDLength;

	for (int i = 0; i < argc; i++) {
		len += sizeof(size_t) + (wcslen(argv[i]) + 1) * sizeof(wchar_t);
	}
	unsigned char * buf = new unsigned char[len];

	int cc = RUN_METHOD;
	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(size_t), &objectID, sizeof(int));
	memcpy(buf + 2 * sizeof(int) + sizeof(size_t), &methodIDLength, sizeof(size_t));
	memcpy(buf + 2 * sizeof(int) + 2 * sizeof(size_t), methodID.c_str(), methodIDLength);

	memcpy(buf + 2 * sizeof(int) + 2 * sizeof(size_t) + methodIDLength, &argc, sizeof(size_t));

	size_t offset = 2 * sizeof(int) + 3 * sizeof(size_t) + methodIDLength;

	for (int i = 0; i < argc; i++) {
		size_t leni = (wcslen(argv[i]) + 1) * sizeof(wchar_t);
		memcpy(buf + offset, &leni, sizeof(size_t));
		memcpy(buf + offset + sizeof(size_t), argv[i], leni);
		offset += sizeof(size_t) + leni;
	}

	BOOL fSuccess = sendBuffer(buf, len, hPipe);

	delete[] buf;

	if (!fSuccess) {
		return NULL;
	}

	// receive reply

	LPVOID result = receiveReturnValue(hPipe, type);

	CloseHandle(hPipe);

	return result;
}

LPVOID receiveReturnValue(HANDLE hPipe, int* type) {
	DWORD cbBytesRead = 0;
	BOOL fSuccess;
	LPVOID result = NULL;

	fSuccess = ReadFile(hPipe, type, sizeof(int), &cbBytesRead, NULL);

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
		return NULL;
	}

	if (*type == INTEGER) {
		int * intResult = new int();
		fSuccess = ReadFile(
			hPipe,        // handle to pipe 
			intResult,    // buffer to receive data 
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
			return NULL;
		}

		result = (LPVOID)intResult;
	}

	if (*type == TCHAR_P) {
		size_t resultLength;
		fSuccess = ReadFile(
			hPipe,        // handle to pipe 
			&resultLength,    // buffer to receive data 
			sizeof(size_t), // size of buffer 
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

		TCHAR * strResult = NULL;

		if (resultLength != 0) {
			strResult = new TCHAR[resultLength / 2];

			fSuccess = ReadFile(
				hPipe,        // handle to pipe 
				strResult,    // buffer to receive data 
				resultLength, // size of buffer 
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
		}

		result = (LPVOID)strResult;

	}

	if (*type == VOID_T) {
		// nothing to return
	}

	if (*type == INVALID_TYPE) {
		printf("Invalid type.\n");
	}

	return result;
}

DWORD sendStringWrapper(LPVOID requestData) {

	Request* request = (Request*)requestData;
	TCHAR * str = request->str;
	double key = request->key;
	TCHAR* pipename = request->pipename;
	sendString(str, key, pipename);
	request->callback((LPVOID)str);

	delete requestData;
	return 0;
}

DWORD sendDoubleWrapper(LPVOID requestData) {

	Request* request = (Request*)requestData;
	double val = request->val;
	double key = request->key;
	TCHAR* pipename = request->pipename;
	sendDouble(val, key, pipename);
	request->callback((LPVOID)&val);

	delete request;
	return 0;
}

DWORD getStringWrapper(LPVOID requestData) {

	Request* request = (Request*)requestData;
	double key = request->key;
	TCHAR* pipename = request->pipename;
	TCHAR* retrievedString;
	getString(key, pipename, &retrievedString);
	request->callback((LPVOID)retrievedString);

	delete request;
	return 0;
}

DWORD getDoubleWrapper(LPVOID requestData) {

	Request* request = (Request*)requestData;
	double val;
	double key = request->key;
	TCHAR* pipename = request->pipename;
	getDouble(key, pipename, &val);
	request->callback((LPVOID)&val);

	delete request;
	return 0;
}

DWORD getObjectAttributeWrapper(LPVOID requestData) {
	ObjectRequest* request = (ObjectRequest*)requestData;
	int objectId = request->objectId;
	std::wstring objectMember = std::wstring(request->objectMemberId);
	TCHAR* pipename = request->pipename;
	int * type = new int();
	LPVOID result = getObjectAttribute(objectId, objectMember, pipename, type);

	ReturnValue returnValue;
	returnValue.type = *type;
	returnValue.result = result;
	request->callback((LPVOID)&returnValue);

	delete request->objectMemberId;
	delete request;
	return 0;
}

LPVOID getObjectAttributeAsync(int objectId, std::wstring attributeID, TCHAR *pipename, int *type, LPVOID(*callback)(LPVOID)){
	HANDLE hHeap = GetProcessHeap();
	ObjectRequest * request = new ObjectRequest();

	request->objectId = objectId;
	request->objectMemberId = new wchar_t[wcslen(attributeID.c_str()) + 1];
	wcscpy_s(request->objectMemberId, wcslen(attributeID.c_str())+1, attributeID.c_str());
	request->pipename = pipename;
	request->callback = callback;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(NULL, 0, getObjectAttributeWrapper, (LPVOID)request, 0, &dwThreadID);

	if (hThread == NULL)
	{
		delete request;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else
	{
		CloseHandle(hThread);
	}
	return 0;
}

DWORD instantiateClassOnServerWrapper(LPVOID requestData) {
	ObjectRequest* request = (ObjectRequest*)requestData;
	std::wstring classId = std::wstring(request->classId);
	TCHAR* pipename = request->pipename;
	int objectID;
	bool result = instantiateClassOnServer(classId, &objectID, pipename);

	request->callback((LPVOID)&objectID);

	delete request->classId;
	delete request;
	return 0;
}

bool instantiateClassOnServerAsync(std::wstring classId, int *objectID, TCHAR *pipename, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	ObjectRequest * request = new ObjectRequest();

	request->classId = new wchar_t[wcslen(classId.c_str()) + 1];
	wcscpy_s(request->classId, wcslen(classId.c_str()) + 1, classId.c_str());
	request->pipename = pipename;
	request->callback = callback;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		instantiateClassOnServerWrapper,    // thread proc
		(LPVOID)request,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		delete request;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return 0;
}

DWORD runMethodOnServerWrapper(LPVOID requestData) {
	ObjectRequest* request = (ObjectRequest*)requestData;
	int objectId = request->objectId;
	size_t argc = request->argc;
	std::wstring objectMember = std::wstring(request->objectMemberId);
	TCHAR* pipename = request->pipename;
	TCHAR ** argv = request->argv;

	int * type = new int();
	LPVOID result = runMethodOnServer(objectId, objectMember, argc, argv, pipename, type);

	ReturnValue returnValue;
	returnValue.type = *type;
	returnValue.result = result;
	request->callback((LPVOID)&returnValue);

	delete request->objectMemberId;
	delete request;
	return 0;
}

LPVOID runMethodOnServerAsync(int objectId, std::wstring methodID, size_t argc, TCHAR** argv, TCHAR *pipename, int *type, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	ObjectRequest * request = new ObjectRequest();

	request->objectId = objectId;
	request->objectMemberId = new wchar_t[wcslen(methodID.c_str()) + 1];
	wcscpy_s(request->objectMemberId, wcslen(methodID.c_str()) + 1, methodID.c_str());
	request->pipename = pipename;
	request->callback = callback;
	request->argc = argc;
	for (int i = 0; i < argc; i++) {
		request->argv[i] = argv[i];
	}

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		runMethodOnServerWrapper,    // thread proc
		(LPVOID)request,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		delete request;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return 0;
}

bool sendString(TCHAR *str, double key, TCHAR *pipename) {
	HANDLE hPipe;
	bool fSuccess;

	hPipe = connect(pipename);

	if (hPipe == NULL) {
		return false;
	}

	// Send a message to the pipe server. 

	int cc = SEND_STRING;

	size_t strLength = (_tcslen(str) + 1) * sizeof(wchar_t);
	size_t len = sizeof(int)+ sizeof(double) + sizeof(size_t) * 2 + strLength;

	unsigned char * buf = new unsigned char[len];

	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + +sizeof(size_t), &key, sizeof(double));
	memcpy(buf + sizeof(int) + +sizeof(size_t) + sizeof(double), &strLength, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(double) + 2*sizeof(size_t), str, strLength);

	fSuccess = sendBuffer(buf, len, hPipe);

	bool ack = false;
	readBooleanFromPipe(hPipe, &ack);

	CloseHandle(hPipe);

	return ack;
}

bool sendDouble(double val, double key, TCHAR *pipename) {
	HANDLE hPipe;
	BOOL fSuccess;

	hPipe = connect(pipename);

	if (hPipe == NULL) {
		return false;
	}

	// Send a message to the pipe server. 

	int cc = SEND_DOUBLE;

	size_t len = sizeof(int) + sizeof(double)*2 + sizeof(size_t);

	unsigned char * buf = new unsigned char[len];

	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(size_t), &key, sizeof(double));
	memcpy(buf + sizeof(int) + sizeof(double) + sizeof(size_t), &val, sizeof(double));

	fSuccess = sendBuffer(buf, len, hPipe);

	bool ack = false;
	readBooleanFromPipe(hPipe, &ack);

	CloseHandle(hPipe);

	return ack;
}

bool getString(double key, TCHAR *pipename, TCHAR ** str) {
	HANDLE hPipe;
	BOOL fSuccess;

	hPipe = connect(pipename);

	if (hPipe == NULL) {
		return false;
	}

	// Send a message to the pipe server. 

	int cc = GET_STRING;
	// make and send request
	size_t len = sizeof(int) + sizeof(size_t) + sizeof(double);
	unsigned char * buf = new unsigned char[len];

	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(size_t), &key, sizeof(double));

	sendBuffer(buf, len, hPipe);

	delete[] buf;


	bool foundValue = false;
	fSuccess = readBooleanFromPipe(hPipe, &foundValue);
	if (!fSuccess) {
		return false;
	}

	if (foundValue == true) {
		fSuccess = readStringFromPipe(hPipe, str);
	}

	CloseHandle(hPipe);
	return foundValue;
}

bool getDouble(double key, TCHAR *pipename, double * val) {
	HANDLE hPipe;

	hPipe = connect(pipename);

	if (hPipe == NULL) {
		return false;
	}

	// make and send request
	size_t len = sizeof(int) + sizeof(size_t) + sizeof(double);
	unsigned char * buf = new unsigned char[len];

	int cc = GET_DOUBLE;
	memcpy(buf, &cc, sizeof(int));
	memcpy(buf + sizeof(int), &len, sizeof(size_t));
	memcpy(buf + sizeof(int) + sizeof(size_t), &key, sizeof(double));

	sendBuffer(buf, len, hPipe);

	delete[] buf;

	// receive reply

	bool foundValue = false;
	readBooleanFromPipe(hPipe, &foundValue);

	if (foundValue == true) {
		readDoubleFromPipe(hPipe, val);
	}

	CloseHandle(hPipe);
	return foundValue;
}

bool sendStringAsync(TCHAR *str, double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	Request * requestData = new Request();

	requestData->callback = callback;
	requestData->pipename = pipename;
	requestData->key = key;
	requestData->str = str;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		sendStringWrapper,    // thread proc
		(LPVOID)requestData,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		delete requestData;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return 0;
}

bool getStringAsync(double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	Request * requestData = (Request*)HeapAlloc(hHeap, 0, sizeof(Request));

	requestData->callback = callback;
	requestData->pipename = pipename;
	requestData->key = key;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		getStringWrapper,    // thread proc
		(LPVOID)requestData,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return 0;
}

bool sendDoubleAsync(double val, double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	Request * requestData = new Request();

	requestData->callback = callback;
	requestData->pipename = pipename;
	requestData->key = key;
	requestData->val = val;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		sendDoubleWrapper,    // thread proc
		(LPVOID)requestData,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		delete requestData;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return 0;
}



bool getDoubleAsync(double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) {
	HANDLE hHeap = GetProcessHeap();
	Request * requestData = (Request*)HeapAlloc(hHeap, 0, sizeof(Request));

	requestData->callback = callback;
	requestData->pipename = pipename;
	requestData->key = key;

	HANDLE hThread;
	DWORD dwThreadID = 0;
	hThread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		getDoubleWrapper,    // thread proc
		(LPVOID)requestData,    // thread parameter 
		0,                 // not suspended 
		&dwThreadID);      // returns thread ID 

	if (hThread == NULL)
	{
		delete requestData;
		_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
		return false;
	}
	else CloseHandle(hThread);

	return true;
}


LPVOID stringCallback(LPVOID input) {
	TCHAR * str = (TCHAR*)input;
	std::wcout << "\n(stringCallback)this string was sent or received: "<<str << std::endl;
	return NULL;
}

LPVOID doubleCallback(LPVOID input) {
	double val = *((double*)input);
	std::wcout << "\n    (doubleCallback)this number was sent or received: " << val << std::endl;
	return NULL;
}

LPVOID instantiateCallback(LPVOID input) {
	std::wcout << "    (Instantiate callback)This is the ID of the asynchronously instantiated object: " << *(int*)input << std::endl;
	return NULL;
}

LPVOID objectCallback(LPVOID input) {
	ReturnValue * returnValue = (ReturnValue*)input;
	std::wcout << "\n    (objectCallback)";
	if (returnValue->type == INTEGER) {
		std::wcout << "Return value is " << *(int*)returnValue->result << std::endl;
	}
	if (returnValue->type == TCHAR_P) {
		std::wcout << "Return value is " << (TCHAR*)returnValue->result << std::endl;
	}
	if (returnValue->type == VOID_T) {
		std::wcout << "Return value is void type." << std::endl;
	}
	if (returnValue->type == INVALID_TYPE) {
		std::wcout << "Invalid type is returned." << std::endl;
	}
	return NULL;
}

int _tmain(int argc, TCHAR *argv[])
{
	HANDLE hHeap = GetProcessHeap();
	size_t msgLength = _tcslen(L"\\\\.\\pipe\\mycpipe") + 1;;
	TCHAR * lpszPipename = new TCHAR[msgLength];
	lstrcpy(lpszPipename, L"\\\\.\\pipe\\mycpipe");
	TCHAR* msg = new TCHAR[18];
	lstrcpy(msg, L"Message1");

	sendString(msg, 1, lpszPipename);
	std::wcout << "\nSent string \"" << msg << "\" to server for key " << 1 << std::endl;
	TCHAR *message;
	bool foundVal=false;
	foundVal = getString(1, lpszPipename, &message);
	if (foundVal) {
		std::wcout << "Found the following message for key "<<1<< ": \""<< message <<"\"\n\n" << std::endl;
	}
	else {
		std::wcout << "Did not find string for key 1." << std::endl;
	}

	for (int i = 0; i < 3; i++) {
		sendDouble(i, i, lpszPipename);

		std::wcout << "Number "<< i <<" sent with key "<< i << "." << std::endl;

		double val;
		bool foundVal;
		foundVal = getDouble(i, lpszPipename, &val);
		if (foundVal) {
			std::cout << "Found value: " << val << " for key: " << i << "\n" << std::endl;
		}
		else {
			std::cout << "Found no value for key " << i << "\n" << std::endl;
		}

		foundVal = getDouble(i+1, lpszPipename, &val);
		if (foundVal) {
			std::cout << "Found value: " << val << " for key: " << i+1 << "\n" << std::endl;
		}
		else {
			std::cout << "Found no value for key " << i+1 << "\n" << std::endl;
		}
	}
	
	int objectId;

	std::wcout << "\nInstantiating a Car object on server.\n" << std::endl;
	instantiateClassOnServer(L"Car", &objectId, lpszPipename);
	std::wcout << "Instantiated a Car object with ID " << objectId << "\n" << std::endl;

	int *type = new int();
	LPVOID result;

	TCHAR * argv2[1];
	argv2[0] = (TCHAR*)HeapAlloc(hHeap, 0, 5 * sizeof(TCHAR));
	lstrcpy(argv2[0], L"2017");
	size_t argcount = 1;
	result = runMethodOnServer(objectId, std::wstring(L"setYear"), argcount, argv2, lpszPipename, type);
	std::wcout << "\nSet year on the server for object " << objectId << " to " << argv2[0] << std::endl;

	result = getObjectAttribute(objectId, std::wstring(L"year"), lpszPipename, type);

	if (result == NULL) {
		std::wcout << L"\n got a null" << std::endl;
	}
	else {
		if (*type == TCHAR_P) {
			std::wcout << (TCHAR*)result << std::endl;
		}
		if (*type == INTEGER) {
			std::wcout << "\nGot year for object " << objectId << " from the server: "<< *(int*)result << std::endl;
		}
	}

	HeapFree(hHeap, 0, argv2[0]);

	argv2[0] = (TCHAR*)HeapAlloc(hHeap, 0, 11 * sizeof(TCHAR));
	lstrcpy(argv2[0], L"Volkswagen");
	result = runMethodOnServer(2, std::wstring(L"setModel"), argcount, argv2, lpszPipename, type);
	std::wcout << "\nSet model on the server for object " << objectId << " to " << argv2[0] << std::endl;
	result = getObjectAttribute(2, std::wstring(L"model"), lpszPipename, type);
	if (result == NULL) {
		std::wcout << L"got a null" << std::endl;
	}
	else {
		if (*type == TCHAR_P) {
			std::wcout << "\nGot the model for object "<<objectId<<": "<<(TCHAR*)result << std::endl;
		}
		if (*type == INTEGER) {
			std::wcout << *(int*)result << std::endl;
		}
	}

	std::wcout << "\n\n----------------\n\nAsynchronous tests:\n" << std::endl;
	lstrcpy(argv2[0], L"Golf");
	std::wcout << "Setting asynchronously make of the car with object id " << objectId << " to Golf " << std::endl;
	result = runMethodOnServerAsync(objectId, std::wstring(L"setMake"), argcount, argv2, lpszPipename, type, objectCallback);
	std::wcout << "\nWaiting 5 seconds" << std::endl;
	Sleep(5000);
	result = getObjectAttributeAsync(objectId, std::wstring(L"make"), lpszPipename, type, objectCallback);
	std::wcout << "\nGetting the newly set make asynchronously." << std::endl;
	std::wcout << "\nWaiting 5 seconds." << std::endl;
	Sleep(5000);

	std::wcout << "\nInstantiating a new car on the server asynchronously.\n" << std::endl;
	instantiateClassOnServerAsync(L"Car", &objectId, lpszPipename, instantiateCallback);
	std::wcout << "\nWaiting 5 seconds for the callback.\n" << std::endl;
	Sleep(5000);

	HeapFree(hHeap, 0, argv2[0]);
	
	if (type != NULL) {
		delete type;
	}
		
	delete [] lpszPipename;
}

