#pragma once
#include <map>
#include <windows.h>
#include <mutex>

class BaseClass {
	// base class used to register different classes and store objects

public:

	BaseClass();
	static std::mutex objectCounterLock;
	static int objectCounter;
	int id;
	static BaseClass * makeObject(std::wstring classId);
	virtual BaseClass* clone();
	static bool registerClass(std::wstring classId, BaseClass* BaseClass);
	static std::map<std::wstring, BaseClass*> classRegistry;
	static std::map<int, BaseClass*> objectRegistry;
	virtual LPVOID reflectMethod(std::wstring methodId, size_t argc, TCHAR** argv, int* type);
	virtual LPVOID reflectAttribute(std::wstring attributeId, int *type);

	virtual int getId() { return this->id; }
};
