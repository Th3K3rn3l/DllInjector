#pragma once

#include <string>

class Injector
{
public:
	Injector() {};
	~Injector() {};

	std::wstring getDllPath();
	void setDllPath(std::wstring absFullPath);
	int getPID();
	void setPID(int pid);

	// Different injection methods
	bool standardInject(); // создание потока в целевом процессе и вызов в нем loadLibrary()
	bool threadHijacking(); // перехват потока целевого процесса и вызов в нем loadLibrary()
	bool manualMap(); // manual mapping
private:
	std::wstring absDllPath;
	int pID;
};