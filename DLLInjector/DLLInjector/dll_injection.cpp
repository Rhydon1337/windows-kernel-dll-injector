#include "dll_injection.h"

#include <ntifs.h>

#include "consts.h"
#include "ProcessReference.h"
#include "apc.h"

using LoadLibraryW = HANDLE(*)(LPCWSTR lpLibFileName);

#pragma optimize("", off)
#pragma runtime_checks("", off )
void user_mode_apc_callback(PVOID args, PVOID, PVOID) {
	auto injection_args = reinterpret_cast<InjectDllArgs*>(args);
	((LoadLibraryW)injection_args->load_library_function_address)(injection_args->dll_path);
}

void user_mode_apc_callback_end() {}
#pragma runtime_checks("", restore)
#pragma optimize("", on)

NTSTATUS inject_dll(const InjectDllArgs& inject_dll_args) {
	PVOID apc_args_allocation = nullptr;
	PVOID apc_callback_allocation = nullptr;
	PKTHREAD target_thread;
	{
		// Attack to target process
		ProcessReference process_reference;
		CHECK(process_reference.init(inject_dll_args.pid, true));

		// Allocate and copy the dll path to target process
		SIZE_T apc_args_allocation_size = sizeof(InjectDllArgs);
		CHECK(ZwAllocateVirtualMemory(NtCurrentProcess(), &apc_args_allocation, 0, &apc_args_allocation_size, MEM_COMMIT, PAGE_READWRITE));
		RtlCopyMemory(apc_args_allocation, &inject_dll_args, apc_args_allocation_size);

		// Allocate and copy the apc user mode callback code to target process
		SIZE_T code_size = reinterpret_cast<ULONG_PTR>(user_mode_apc_callback_end) - reinterpret_cast<ULONG_PTR>(user_mode_apc_callback);
		ZwAllocateVirtualMemory(NtCurrentProcess(), &apc_callback_allocation, NULL, &code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		RtlCopyMemory(apc_callback_allocation, &user_mode_apc_callback, code_size);

		target_thread = KeGetCurrentThread();
	}
	// Execute LoadLibrary in the target process in order to load our dll
	call_apc(target_thread, inject_dll_args.load_library_function_address, apc_args_allocation);
	return STATUS_SUCCESS;
}
