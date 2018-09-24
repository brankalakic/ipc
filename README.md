# Project Description

This project implements a server and a client application for coding challenge: link.
The server was based on the multithreaded server with named pipes as provided here: link.

The server runs an infinite thread and waits for client commands. As they arrive, it creates a thread for each of the commands. After the command is performed, the thread exits. 

The client can connect and make synchronous and asynchronous calls to the server. It makes asynchronous calls to the server by generating a new thread and running a command. Synchronous calls run in the calling thread.

Methods for sending strings and doubles are implemented on the client as well as method for getting them from the server. Strings and doubles are stored at the server in two maps (one for each data type). The maps are indexed using keys of type double.

Pipename can be passed as an input variable to all the commands. Right now pipename is hard-coded in the main at the client side. Server starts a pipe in its main with a hard-coded name as well.

Client can instantiate classes on the server that have been registered at the server. Each objects must inherit from a class BaseClass that was implemented as part of the project. When a class is registered, an object is created from it and stored into a map. A client instinatiates an object by cloning that initial object and storing it in a different map.

Client can access methods and attributes on the server through virtual methods reflectMethod and reflectAttribute. These methods must be implemented for each registered class.

Class Car was created and inherits the BaseClass. This class is used to demonstrate object instantiation and running methods and getting attributes on the server remotely from the client.

# Building and Running Server and Client

Firs step is switching to the project directory ipc. Once there follow these steps:
``` bash
mkdir bin
cd bin
cmake -G "Visual Studio 15 2017 Win64" ..
cmake --build .
```

This will generate executables both for client and server:
``` bash
bin/src/client/Debug/client.exe
bin/src/server/Debug/server.exe
```

# Testing
## Current main
Current main on the client side tests the various functionalities of the code. 

The server registers the class Car. 

The client first sends and gets a string from the server. Then it sends some values to the server and retrieves them. It also attempts to retrieve some of the values before they are sent and prints out the appropriate messages.

Next, client instantiates a Car object on the server and sets some of the object attributes by calling setter methods. Then it retrieves the values of the set attributes by using object attribute retrieval functionality.

The last part of the test demonstrates the work of the asynchronous methods. Asynchronous version of all seven commands are implemented, but here only object instantiation, remote method running and attribute retrieval are demonstrated. After each call to the server a timer is started to ensure that the program does not exit before the asynchronous calls finish and their respective callbacks are called.

## Commands
### sendString
```c++
// str is the string to be sent and stored on the server
// key is the key at which to store the string.
// pipename is the name of the pipe to connect to.
bool sendString(TCHAR *str, double key, TCHAR *pipename);
```
### sendDouble
```c++
// Similar to sendString with the difference that val
// is the value that is sent over to the server.
bool sendDouble(double val, double key, TCHAR *pipename);
```
### getString
```c++
// A function for retrieving a string from the server
// stored in a map with key having the value key.
//
// pipename is the name of the pipe to connect to.
// str is the variable in which the retrieved string will be
// stored.
// Returned value is true if a string exists at the server
// at key key.
bool getString(double key, TCHAR *pipename, TCHAR ** str);
```
### getDouble
```c++
// Similar to getString.
bool getDouble(double key, TCHAR *pipename, double * val);
```
### instantiateClassOnServer
```c++
// Instantiates a class on the server whose class is classId.
//
// objectID is a pointer to where the id of the instantiated object will be stored
// if the operation is successful.
// pipename is the name of the pipe to connect to.
bool instantiateClassOnServer(std::wstring classId, int *objectID, TCHAR *pipename);
```
### getObjectAttribute
```c++
// objectId is ID of the object we want to access.
// attributeID is the id for the object we want to access.
// pipename is the name of the pipe to connect to.
// *type will contain the type information of the returned attribute.
// Return value is LPVOID and it is a pointer to the retrieved attribute.
LPVOID getObjectAttribute(int objectId, std::wstring attributeID, TCHAR *pipename, int *type);
```
### runMethodOnServer
```c++
// objectId is ID of the object we want to access.
// methodID is the id of the method that we want to run on the object.
// argc is the number of arguments to the method.
// argv is an array of arguments to the method.
// *type will contain the type information of the returned attribute.
// pipename is the name of the pipe to connect to
// Return value is an LPVOID and is a pointer to the return value of the function.
LPVOID runMethodOnServer(int objectID, std::wstring methodID, size_t argc, TCHAR **argv, TCHAR *pipename, int*type);
```

Each of the methods has an asynchronous counterpart. The only difference in the function signature apart from the name is the callback function that is passed as an input parameter.

### sendStringAsync
```c++
bool sendStringAsync(TCHAR *str, double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) ;
```
### sendDoubleAsync
```c++
bool sendDoubleAsync(double val, double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) ;
```
### getDoubleAsync
```c++
bool getDoubleAsync(double key, TCHAR *pipename, LPVOID(*callback)(LPVOID));
```
### getStringAsync
```c++
bool getStringAsync(double key, TCHAR *pipename, LPVOID(*callback)(LPVOID)) ;
```
### instantiateClassOnServerAsync
```c++
bool instantiateClassOnServerAsync(std::wstring classId, int *objectID, TCHAR *pipename, LPVOID(*callback)(LPVOID));
```
### getObjectAttributeAsync
```c++
LPVOID getObjectAttributeAsync(int objectId, std::wstring attributeID, TCHAR *pipename, int *type, LPVOID(*callback)(LPVOID));
```
### runMethodOnServerAsync
```c++
LPVOID runMethodOnServerAsync(int objectId, std::wstring methodID, size_t argc, TCHAR** argv, TCHAR *pipename, int *type, LPVOID(*callback)(LPVOID));
```