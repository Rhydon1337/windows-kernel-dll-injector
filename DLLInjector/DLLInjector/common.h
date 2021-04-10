#pragma once

struct InjectDllArgs {
	size_t pid;
	wchar_t dll_path[256];
};

#define INJECT_DLL_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1337, METHOD_BUFFERED, FILE_WRITE_DATA)