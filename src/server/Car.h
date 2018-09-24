#pragma once

#include "windows.h"
#include "BaseClass.h"
#include <string>
#include "../common/utils.h"

class Car : public BaseClass {
	// Example class to demonstrate object creation and manipulation on the server from the client.

	TCHAR * model;
	TCHAR * make;
	int id;
	int year;

public:
	Car();

	~Car();

	void setModel(TCHAR* model);

	void setMake(TCHAR* make);

	void setYear(int year);

	int getId() override;

	BaseClass * clone();

	LPVOID reflectMethod(std::wstring methodId, size_t argc, TCHAR** argv, int *type);

	LPVOID reflectAttribute(std::wstring attributeId, int * type);

};
