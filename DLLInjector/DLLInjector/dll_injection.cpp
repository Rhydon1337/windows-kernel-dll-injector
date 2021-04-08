#include "dll_injection.h"

#include <ntifs.h>

#include "consts.h"
#include "ProcessReference.h"
#include "apc.h"
#include "process.h"

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
	PVOID injected_apc_args = nullptr;
	PVOID injected_apc_callback = nullptr;
	ProcessInfo process_info;
	PKTHREAD target_thread;
	{
		// Attack to target process
		ProcessReference process_reference;
		CHECK(process_reference.init(inject_dll_args.pid, true));

		// Allocate and copy the dll path to target process
		SIZE_T apc_args_allocation_size = sizeof(InjectDllArgs);
		CHECK(ZwAllocateVirtualMemory(NtCurrentProcess(), &injected_apc_args, 0, &apc_args_allocation_size, MEM_COMMIT, PAGE_READWRITE));
		RtlCopyMemory(injected_apc_args, &inject_dll_args, apc_args_allocation_size);

		// Allocate and copy the apc user mode callback code to target process
		SIZE_T code_size = reinterpret_cast<ULONG_PTR>(user_mode_apc_callback_end) - reinterpret_cast<ULONG_PTR>(user_mode_apc_callback);
		if (!NT_SUCCESS(ZwAllocateVirtualMemory(NtCurrentProcess(), &injected_apc_callback, NULL, &code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE))) {
			goto release_apc_callback_args;
		}
		
		RtlCopyMemory(injected_apc_callback, &user_mode_apc_callback, code_size);

		if (!NT_SUCCESS(get_process_info_by_pid(inject_dll_args.pid, &process_info))) {
			goto release_apc_callback;
		}

		if (!NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)process_info.threads_id[0], &target_thread))) {
			goto release_process_info;
		}

		goto success;
		release_process_info:
		ExFreePool(process_info.threads_id);
		release_apc_callback:
		ZwFreeVirtualMemory(NtCurrentProcess(), &injected_apc_callback, &code_size, MEM_RELEASE);
		release_apc_callback_args:
		ZwFreeVirtualMemory(NtCurrentProcess(), &injected_apc_args, &apc_args_allocation_size, MEM_RELEASE);
		return STATUS_UNSUCCESSFUL;
		success:
		KdPrint(("Finished all allocations, start the apc injection\n"));
	}
	// Execute LoadLibrary in the target process in order to load our dll
	call_apc(target_thread, injected_apc_callback, injected_apc_args);
	ExFreePool(process_info.threads_id);
	ObDereferenceObject(target_thread);
	return STATUS_SUCCESS;
}
