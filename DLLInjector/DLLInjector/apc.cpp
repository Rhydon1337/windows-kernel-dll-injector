#include "apc.h"

VOID KernelAPC(PVOID context, PVOID, PVOID, PVOID, PVOID) {
    ExFreePool(context);
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

    KeInsertQueueApc(apc, 0, NULL, 0);

	return STATUS_SUCCESS;
}
