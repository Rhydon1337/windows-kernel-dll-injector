#include "apc.h"

VOID KernelAPC(PVOID, PVOID, PVOID, PVOID, PVOID) {
}

NTSTATUS call_apc(PKTHREAD target_thread, PVOID target_function, PVOID params) {
    KAPC* apc = static_cast<KAPC*>(ExAllocatePool(NonPagedPool, sizeof(KAPC)));
    if (nullptr == apc) {
        return STATUS_UNSUCCESSFUL;
    }
    KeInitializeApc(apc,
        target_thread,
        OriginalApcEnvironment,
        reinterpret_cast<PKKERNEL_ROUTINE>(KernelAPC),
        nullptr,
        reinterpret_cast<PKNORMAL_ROUTINE>(target_function),
        UserMode,
        params);

    if (!KeInsertQueueApc(apc, nullptr, nullptr, 0)) {
        ExFreePool(apc);
    	return STATUS_UNSUCCESSFUL;
    }
	return STATUS_SUCCESS;
}