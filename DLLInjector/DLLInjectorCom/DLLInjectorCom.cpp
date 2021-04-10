#include <Windows.h>
#include <iostream>
#include <string>

#include "../DLLInjector/common.h"

int wmain(int, wchar_t** argv) {
    std::cout << "Hello World!\n";
	HANDLE driver = CreateFileA("\\\\.\\DLLInjector", GENERIC_ALL, 0, 
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == driver) {
		return 1;
    }
	InjectDllArgs args = {};
	memcpy(args.dll_path, argv[1], wcslen(argv[1]) * sizeof(wchar_t));
	args.pid = std::stoi(argv[2]);

	DeviceIoControl(driver, INJECT_DLL_IOCTL, &args, sizeof(InjectDllArgs),
		nullptr, 0, nullptr, nullptr);

	CloseHandle(driver);
	return 0;
}