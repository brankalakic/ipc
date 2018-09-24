#pragma once

#include <windows.h> 
#include <string>
#include <tchar.h>
#include <iostream>
#include <map>

#define MAX_ARGS 20

#define INVALID_TYPE -1

#define SEND_DOUBLE 0
#define SEND_STRING 1
#define GET_DOUBLE 2
#define GET_STRING 3
#define INSTANTIATE_OBJECT 4
#define GET_OBJECT_ATTRIBUTE 5
#define RUN_METHOD 6

#define INTEGER 7
#define TCHAR_P 8
#define VOID_T 9

bool readStringFromPipe(HANDLE hPipe, TCHAR** message); 

bool readDoubleFromPipe(HANDLE hPipe, double *key);

bool readBooleanFromPipe(HANDLE hPipe, bool *val);

bool writeBooleanToPipe(HANDLE hPipe, bool val);

bool readIntFromPipe(HANDLE hPipe, int *objectID);

BOOL sendBuffer(unsigned char *buf, size_t len, HANDLE hPipe);

struct Request {
	TCHAR * str;
	double key;
	double val;
	TCHAR * pipename;
	LPVOID(*callback)(LPVOID l);
};

struct ObjectRequest {
	int objectId;
	wchar_t* objectMemberId;
	wchar_t* classId;
	TCHAR * pipename;
	size_t argc;
	TCHAR *argv[MAX_ARGS];
	LPVOID(*callback)(LPVOID l);
};

struct ReturnValue{
	int type;
	LPVOID result;
};








