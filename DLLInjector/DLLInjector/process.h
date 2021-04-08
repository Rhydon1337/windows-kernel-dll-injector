#pragma once
#include <wdm.h>

struct ProcessInfo {
	size_t process_id;
	size_t number_of_threads;
	size_t* threads_id;
};

ProcessInfo* get_processes_info(size_t* number_of_processes);

NTSTATUS get_process_info_by_pid(size_t pid, ProcessInfo* process_info);