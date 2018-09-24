#include "BaseClass.h"

std::map<std::wstring, BaseClass *> BaseClass::classRegistry;
std::map<int, BaseClass*> BaseClass::objectRegistry;
int BaseClass::objectCounter = 0;
std::mutex BaseClass::objectCounterLock;

BaseClass::BaseClass() {
	BaseClass::objectCounterLock.lock();
	BaseClass::objectCounter++;
	BaseClass::objectCounterLock.unlock();
}

BaseClass * BaseClass::makeObject(std::wstring classId) {
	if (BaseClass::classRegistry.count(classId) == 0) {
		return NULL;
	}
	else
	{
		return BaseClass::classRegistry[classId]->clone();
	}
}

BaseClass* BaseClass::clone() {
	return NULL;
}

LPVOID BaseClass::reflectMethod(std::wstring methodId, size_t argc, TCHAR** argv, int* type) {
	return NULL;
}

LPVOID BaseClass::reflectAttribute(std::wstring attributeId, int* type) {
	return NULL;
}

bool BaseClass::registerClass(std::wstring classId, BaseClass* BaseClass) {
	BaseClass::classRegistry[classId] = BaseClass;
	return true;
}
