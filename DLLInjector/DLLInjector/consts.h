#pragma once

#define NT_DEVICE_NAME L"\\Device\\DLLInjector"
#define DOS_DEVICE_NAME L"\\DosDevices\\DLLInjector"

#define CHECK(ntstatus) if (!NT_SUCCESS(ntstatus)) { return ntstatus;}

#define DELETE_DEFAULT_CTORS(class_name) \
	class_name(class_name const&) = delete; \
	class_name& operator =(class_name const&) = delete; \
	class_name(class_name&&) = delete; \
	class_name& operator=(class_name&&) = delete \