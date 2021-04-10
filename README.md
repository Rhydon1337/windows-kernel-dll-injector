# windows-kernel-dll-injector
## TL;DR
Windows kernel mode to user mode dll injection

Tested on Windows x64 1909

Inject a dll to target process from kernel driver

## How its works
The injection process is divided into several stages:

1. Attach current kernel thread to the virtual address space of the target process (KeStackAttachProcess)
2. Parse kernel32.dll PE header and locate LoadLibraryW function address
	* Get current process PEB (PsGetProcessPeb)
	* Iterate over all loaded modules and find kernel32.dll
	* Parse kernel32.dll PE header in order to find the address of LoadLibraryW
3. Allocate and copy to target process the APC callback arguments RW (ZwAllocateVirtualMemory)
4. Allocate and copy to target process the APC callback RWX (ZwAllocateVirtualMemory)
5. Detach from target process address space (KeUnstackDetachProcess)
6. Find all target process threads
	* ZwQuerySystemInformation(SystemProcessInformation...)
	* Iterate over all processes and find the target process by its id
	* Return all threads id of the target process
7. Inject APC to target process in order to execute our APC callback which loads the dll (KeInitializeApc & KeInsertQueueApc)

The whole process described above happens in the kernel driver. The only things that the kernel module needs are: target pid, dll file path.

## Limitations
* The current version supports only in x64 binaries
* The current version doesn't release the APC callback or the APC callback arguments allocations

## Usage

sc create DllInjector binPath= {driver_path} type= kernel

sc start DllInjector

DLLInjectorCom.exe {dll_path} {pid}

DONE!!!
