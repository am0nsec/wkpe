/*+================================================================================================
Module Name: main.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
Device driver entry point.

================================================================================================+*/

#include "mmanager-globals.h"
#include "mmanager-dispatch.h"

#include "rtl/osversion.h"

/// <summary>
/// Device driver entry point.
/// </summary>
/// <param name="DriverObject">Pointer to the driver object.</param>
/// <param name="RegistryPath">Pointer to the registry path, if any.</param>
/// <returns>Functions status code.</returns>
EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#endif // ALLOC_PRAGMA

_Use_decl_annotations_
EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
) {
	UNREFERENCED_PARAMETER(RegistryPath);

	MMDebug(("----------------------------------------------------------------\r\n"));
	MMDebug(("Kernel Memory MAnager Utility (MManager)                        \r\n"));
	MMDebug(("Version: 1.0                                                    \r\n"));
	MMDebug(("                                                                \r\n"));
	MMDebug(("Tested on: Windows 10 Kernel Version 19041 MP (1 procs) Free x64\r\n"));
	MMDebug(("Edition build lab: 19041.1.amd64fre.vb_release.191206-1406      \r\n"));
	MMDebug(("----------------------------------------------------------------\r\n"));

	// 1. Check current Windows version
	OSVERSIONINFOW SystemInfo = { 0x00 };
	ASSERT(NT_SUCCESS(RtlGetVersion(&SystemInfo)));
	ASSERT(SystemInfo.dwMajorVersion == WINDOWS_10);
	ASSERT(SystemInfo.dwBuildNumber  == WINDOWS_10_19044);

	// Initialise stack variables
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING DeviceName   = RTL_CONSTANT_STRING(MMANAGER_DEVICE_NAME);
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(MMANAGER_SYMBOLIC_LINK_NAME);

	PDEVICE_OBJECT DeviceObject = NULL;
	BOOLEAN        SymbolicLink = FALSE;
	
	// Set all the routines
	DriverObject->DriverUnload = MmanDriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = MmanDriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = MmanDriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MmanDriverDispatch;

	// Register the device driver and symbolic link
	do {
		// Exclusive  - Prevent multiple handles to be open for the device driver.
		// SDDLString - Prevent non-admin and non-SYSTEM from interacting with the device driver.
		Status = WdmlibIoCreateDeviceSecure(
			DriverObject,
			0x00,
			&DeviceName,
			FILE_DEVICE_UNKNOWN,
			0x00,
			TRUE,
			&SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
			&MMANAGER_CLASS_GUID,
			&DeviceObject
		);
		if (!NT_SUCCESS(Status)) {
			MMDebug(("Failed to create device object (0x%08X).\r\n", Status));
			break;
		}
		DeviceObject->Flags |= DO_DIRECT_IO;

		Status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);
		if (!NT_SUCCESS(Status)) {
			MMDebug(("Failed to symbolic link (0x%08X).\r\n", Status));
			break;
		}
		SymbolicLink = TRUE;
	} while (FALSE);

	// Check for errors
	if (!NT_SUCCESS(Status)) {
		MMDebug(("Roll-back operations.\r\n"));
		if (SymbolicLink)
			IoDeleteSymbolicLink(&SymbolicName);
		if (DeviceObject != NULL)
			IoDeleteDevice(DeviceObject);
		MMDebug(("----------------------------------------------------------------\r\n"));
		return Status;
	}

	// Display final information and exit
	MMDebug(("Device name       : %wZ\r\n", DeviceName));
	MMDebug(("Symbolic link name: %wZ\r\n", SymbolicName));
	MMDebug(("----------------------------------------------------------------\r\n"));
	return Status;
}