#include <ntifs.h>

#include "device_handlers.h"
#include "consts.h"

void driver_unload(PDRIVER_OBJECT driver_object) {
	UNICODE_STRING  win32_name;
	RtlInitUnicodeString(&win32_name, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&win32_name);
	if (nullptr != driver_object->DeviceObject) {
		IoDeleteDevice(driver_object->DeviceObject);
	}
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("Hello from driver entry\n");

	UNICODE_STRING  nt_name;
	UNICODE_STRING  win32_name;
	PDEVICE_OBJECT  device_object = nullptr;

	RtlInitUnicodeString(&nt_name, NT_DEVICE_NAME);
	NTSTATUS nt_status = IoCreateDevice(DriverObject, 0, &nt_name,
		FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
		TRUE, &device_object);

	if (!NT_SUCCESS(nt_status)) {
		DbgPrint("Couldn't create the device object\n");
		return nt_status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = device_create_close;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = device_create_close;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = device_ioctl;
	DriverObject->DriverUnload = driver_unload;

	RtlInitUnicodeString(&win32_name, DOS_DEVICE_NAME);

	nt_status = IoCreateSymbolicLink(&win32_name, &nt_name);
	if (!NT_SUCCESS(nt_status)) {
		DbgPrint("Couldn't create symbolic link\n");
		IoDeleteDevice(device_object);
	}

	return nt_status;
}