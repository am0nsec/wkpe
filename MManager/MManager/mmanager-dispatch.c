/*+================================================================================================
Module Name: mmanager-dispatch.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
This file contains all global definition and information used by the kernel driver.

================================================================================================+*/

#include "mmanager-dispatch.h"
#include "mmanager-routines.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MmanDriverUnload)
#pragma alloc_text(PAGE, MmanDriverCreateClose)
#pragma alloc_text(PAGE, MmanDriverDispatch)
#endif // ALLOC_PRAGMA

_Use_decl_annotations_
EXTERN_C VOID MmanDriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
) {
	MMDebug(("----------------------------------------------------------------\r\n\r\n"));

	// Delete the symbolic link and the device object
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(MMANAGER_SYMBOLIC_LINK_NAME);
	IoDeleteSymbolicLink(&SymbolicName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

_Use_decl_annotations_
EXTERN_C NTSTATUS MmanDriverCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	MMDebug(("MmanDriverCreateClose: 0x%0x8\r\n", HandleToUlong(PsGetCurrentProcessId())));

	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0x00;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXTERN_C NTSTATUS MmanDriverDispatch(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	// Get the IRP Stack
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

	ULONG_PTR Information = 0x00;
	NTSTATUS  Status = STATUS_SUCCESS;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_MMANAGER_FIND_PROCESS_VADS:
		Status = MmanIoctlFindProcessVads(Irp, Stack);
		if (NT_SUCCESS(Status))
			Information = sizeof(ULONG64);
		break;
	case IOCTL_MMANAGER_GET_PROCESS_VADS:
		Status = MmanIoctlGetProcessVads(Irp, Stack, &Information);
		if (!NT_SUCCESS(Status))
			Information = 0x00;
		break;
	default:
		MMDebug(("Invalid IOCTL: 0x%08x\r\n", Stack->Parameters.DeviceIoControl.IoControlCode));
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	// Complete request
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}