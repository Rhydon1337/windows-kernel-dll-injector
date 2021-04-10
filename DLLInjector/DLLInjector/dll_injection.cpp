#include "dll_injection.h"

#include <ntifs.h>

#include "consts.h"
#include "ProcessReference.h"
#include "apc.h"
#include "process.h"
#include "pe.h"

using LoadLibraryW = HANDLE(*)(LPCWSTR lpLibFileName);

struct UserApcArgs {
	wchar_t dll_path[256];
	LoadLibraryW load_library;
};

#pragma optimize("", off)
#pragma runtime_checks("", off )
void user_mode_apc_callback(UserApcArgs* args, PVOID, PVOID) {
	args->load_library(args->dll_path);
}

void user_mode_apc_callback_end() {}
#pragma runtime_checks("", restore)
#pragma optimize("", on)


NTSTATUS inject_dll(const InjectDllArgs& inject_dll_args) {
	PVOID injected_apc_args = nullptr;
	PVOID injected_apc_callback = nullptr;
	ProcessInfo process_info;
	PKTHREAD target_thread;
	{
		// Attack to target process
		ProcessReference process_reference;
		CHECK(process_reference.init(inject_dll_args.pid, true));

		UserApcArgs user_apc_args;
		memcpy(&user_apc_args.dll_path, &inject_dll_args.dll_path, 256);
		user_apc_args.load_library = (LoadLibraryW)get_module_symbol_address((wchar_t*)L"KERNEL32.DLL", "LoadLibraryW");
		if (nullptr == user_apc_args.load_library) {
			return STATUS_UNSUCCESSFUL;
		}
		
		// Allocate and copy the dll path to target process
		SIZE_T apc_args_allocation_size = sizeof(UserApcArgs);
		CHECK(ZwAllocateVirtualMemory(NtCurrentProcess(), &injected_apc_args, 0, &apc_args_allocation_size, MEM_COMMIT, PAGE_READWRITE));
		RtlCopyMemory(injected_apc_args, &user_apc_args, sizeof(UserApcArgs));

		// Allocate and copy the apc user mode callback code to target process
		SIZE_T code_size = reinterpret_cast<ULONG_PTR>(user_mode_apc_callback_end) - reinterpret_cast<ULONG_PTR>(user_mode_apc_callback);
		if (!NT_SUCCESS(ZwAllocateVirtualMemory(NtCurrentProcess(), &injected_apc_callback, NULL, &code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE))) {
			ZwFreeVirtualMemory(NtCurrentProcess(), &injected_apc_args, &apc_args_allocation_size, MEM_RELEASE);
			return STATUS_UNSUCCESSFUL;
		}
		
		RtlCopyMemory(injected_apc_callback, &user_mode_apc_callback, reinterpret_cast<ULONG_PTR>(user_mode_apc_callback_end) - reinterpret_cast<ULONG_PTR>(user_mode_apc_callback));
	}

	CHECK(get_process_info_by_pid(inject_dll_args.pid, &process_info));

	for (size_t i = 0; i < process_info.number_of_threads; i++) {
		if (!NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)process_info.threads_id[i], &target_thread))) {
			ExFreePool(process_info.threads_id);
			return STATUS_UNSUCCESSFUL;
		}

		// Execute LoadLibrary in the target process in order to load our dll
		call_apc(target_thread, injected_apc_callback, injected_apc_args);
		ObDereferenceObject(target_thread);
	}
	return STATUS_SUCCESS;
}
