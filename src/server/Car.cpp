#include "Car.h"


Car::Car() {
	this->id = BaseClass::objectCounter;
	this->model = NULL;
	this->make = NULL;
	this->year = 2018;
}

Car::~Car() {
	if (this->model != NULL) {
		delete[] this->model;
	}
	if (this->make != NULL) {
		delete[] this->make;
	}
}

void Car::setModel(TCHAR* model) {
	if (this->model != NULL) {
		delete[]this->model;
	}
	this->model = new TCHAR[(wcslen(model) + 1)];
	_tcscpy_s(this->model, wcslen(model) + 1, model);

}

void Car::setMake(TCHAR* make) {
	if (this->make != NULL) {
		delete[]this->make;
	}
	this->make = new TCHAR[(wcslen(make) + 1)];
	_tcscpy_s(this->make, wcslen(make) + 1, make);
}

void Car::setYear(int year) {
	this->year = year;
}

int Car::getId() { return this->id; }


BaseClass * Car::clone() {
	BaseClass * BaseClass = new Car();
	return BaseClass;
}

LPVOID Car::reflectMethod(std::wstring methodId, size_t argc, TCHAR** argv, int *type) {
	if (_tcscmp(methodId.c_str(), L"setModel") == 0) {
		*type = VOID_T;
		if (argc > 0) {
			this->setModel(argv[0]);
		}
		return NULL;
	}
	if (_tcscmp(methodId.c_str(), L"setMake") == 0) {
		*type = VOID_T;
		if (argc > 0) {
			this->setMake(argv[0]);
		}
		return NULL;
	}
	if (_tcscmp(methodId.c_str(), L"setYear") == 0) {
		*type = VOID_T;
		if (argc > 0) {
			this->setYear(_ttoi(argv[0]));
		}
		return NULL;
	}

	*type = INVALID_TYPE;
	return NULL;
}

LPVOID Car::reflectAttribute(std::wstring attributeId, int * type) {
	if (_tcscmp(attributeId.c_str(), L"model") == 0) {
		*type = TCHAR_P;
		return (LPVOID)model;
	}
	if (_tcscmp(attributeId.c_str(), L"make") == 0) {
		*type = TCHAR_P;
		return (LPVOID)make;
	}
	if (_tcscmp(attributeId.c_str(), L"year") == 0) {
		*type = INTEGER;
		return (LPVOID)&year;
	}
	*type = INVALID_TYPE;
	return NULL;
}