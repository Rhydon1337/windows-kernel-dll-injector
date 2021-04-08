#pragma once
#include <ntifs.h>


NTSTATUS device_create_close(PDEVICE_OBJECT device_object, PIRP irp);

NTSTATUS device_ioctl(PDEVICE_OBJECT device_object, PIRP irp);