#include "process.h"

#include <wdm.h>

#define SystemProcessInformation 5

extern "C" NTSTATUS NTAPI ZwQuerySystemInformation(IN size_t SystemInformationClass,
                                                   OUT PVOID SystemInformation,
                                                   IN ULONG SystemInformationLength,
                                                   OUT PULONG ReturnLength OPTIONAL);
typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    // Padding here in 64-bit
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;

struct SYSTEM_THREAD_INFORMATION {
    ULONGLONG KernelTime;
    ULONGLONG UserTime;
    ULONGLONG CreateTime;
    ULONG WaitTime;
    // Padding here in 64-bit
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitchCount;
    ULONG State;
    KWAIT_REASON WaitReason;
};

typedef struct _IO_COUNTERS {
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;

struct SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    ULONGLONG WorkingSetPrivateSize;
    ULONG HardFaultCount;
    ULONG Reserved1;
    ULONGLONG CycleTime;
    ULONGLONG CreateTime;
    ULONGLONG UserTime;
    ULONGLONG KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE ProcessId;
    HANDLE ParentProcessId;
    ULONG HandleCount;
    ULONG Reserved2[2];
    // Padding here in 64-bit
    VM_COUNTERS VirtualMemoryCounters;
    size_t Reserved3;
    IO_COUNTERS IoCounters;
    SYSTEM_THREAD_INFORMATION Threads[1];
};

PVOID get_all_processes() {
    size_t processes_allocation_size = 0;
    PVOID processes_pool = nullptr;
	
    while (true) {
        processes_allocation_size += 0x10000;
        processes_pool = ExAllocatePool(PagedPool, processes_allocation_size);

        auto status = ZwQuerySystemInformation(SystemProcessInformation, processes_pool, (ULONG)processes_allocation_size, nullptr);
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            ExFreePool(processes_pool);
        }
        else {
            break;
        }
    }
    return processes_pool;
}

NTSTATUS get_process_info_by_pid(size_t pid, ProcessInfo* process_info) {
    size_t number_of_processes = 0;
    ProcessInfo* processes = get_processes_info(&number_of_processes);
    if (nullptr == processes) {
        return STATUS_UNSUCCESSFUL;
    }
    for (size_t i = 0; i < number_of_processes; i++) {
	    if (pid == (processes + i)->process_id) {
            *process_info = *(processes + i);
            (processes + i)->threads_id = nullptr;
	    }
    }
    for (size_t i = 0; i < number_of_processes; i++) {
    	if (nullptr != (processes + i)->threads_id && (processes + i)->number_of_threads > 0) {
            ExFreePool((processes + i)->threads_id);
    	}
    }
    ExFreePool(processes);
	return STATUS_SUCCESS;
}

ProcessInfo* get_processes_info(size_t* number_of_processes) {
    PVOID all_processes = get_all_processes();
    if (nullptr == all_processes) {
        return nullptr;
    }
    *number_of_processes = 0;
    for (auto process = (SYSTEM_PROCESS_INFORMATION*)all_processes; process->NextEntryOffset != 0;
        process = (SYSTEM_PROCESS_INFORMATION*)((char*)process + process->NextEntryOffset)) {
        *number_of_processes += 1;
    }

    auto processes_info = (ProcessInfo*)ExAllocatePool(PagedPool, sizeof(ProcessInfo) * *number_of_processes);

    size_t i = 0;
    for (auto process = (SYSTEM_PROCESS_INFORMATION*)all_processes; process->NextEntryOffset != 0;
        process = (SYSTEM_PROCESS_INFORMATION*)((char*)process + process->NextEntryOffset), ++i) {
        (processes_info + i)->process_id = (size_t)process->ProcessId;
		(processes_info + i)->number_of_threads = (size_t)process->NumberOfThreads;
        if (0 == process->NumberOfThreads) {
            continue;
        }
    	(processes_info + i)->threads_id = (size_t*)ExAllocatePool(PagedPool, sizeof(size_t) * process->NumberOfThreads);
    	for (size_t j = 0; j < process->NumberOfThreads; j++) {
            *(((ProcessInfo*)(processes_info + i))->threads_id + j) = (size_t)process->Threads[j].ClientId.UniqueThread;
        }
    }
	
    ExFreePool(all_processes);
    return processes_info;
}
