/*+================================================================================================
Module Name: main.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Kernel device driver entry point.
================================================================================================+*/

#include "mpp.hpp"
#include "worker.hpp"

/// @brief 
/// @param DriverObject 
/// @param RegistryPath 
/// @return 
EXTERN_C NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

/// @brief 
/// @param DriverObject 
EXTERN_C VOID
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
);

/// @brief The callback routine for IRP_MJ_CREATE and IRP_MJ_CLOSE.
/// @param DeviceObject Caller-supplied pointer to a DEVICE_OBJECT structure.This is the device object for the target device, previously created by the driver's AddDevice routine.
/// @param Irp          Caller-supplied pointer to an IRP structure that describes the requested I/O operation.
/// @return If the routine succeeds, it must return STATUS_SUCCESS. Otherwise, it must return one of the error status values defined in Ntstatus.h.
EXTERN_C NTSTATUS
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
_IRQL_requires_max_(PASSIVE_LEVEL)
DriverCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
);

/// @brief The callback routine services various IRPs. In this case handle user-mode IOCTL. For a list of function codes, see mpp.h.
/// @param DeviceObject Caller-supplied pointer to a DEVICE_OBJECT structure.This is the device object for the target device, previously created by the driver's AddDevice routine.
/// @param Irp          Caller-supplied pointer to an IRP structure that describes the requested I/O operation.
/// @return If the routine succeeds, it must return STATUS_SUCCESS. Otherwise, it must return one of the error status values defined in Ntstatus.h.
EXTERN_C NTSTATUS
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
_IRQL_requires_max_(DISPATCH_LEVEL)
DriverDispatch(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DriverCreateClose)
#pragma alloc_text(PAGE, DriverDispatch)
#endif // ALLOC_PRAGMA


_Use_decl_annotations_
EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
) {
	UNREFERENCED_PARAMETER(RegistryPath);

	MppLogger::Info("=================================================================\r\n");
	MppLogger::Info("Module Name: Memory Patching Protection (MPP)                    \r\n");
	MppLogger::Info("Author     : Paul L. (@am0nsec)                                  \r\n");
	MppLogger::Info("Origin     : https://github.com/am0nsec/wkpe/                    \r\n");
	MppLogger::Info("Tested OS  : Windows 10 (20h2) - 19044.2006                      \r\n");
	MppLogger::Info("=================================================================\r\n");

	// Do OS check before going forward
	OSVERSIONINFOW SystemInfo = { 0x00 };
	ASSERT(NT_SUCCESS(RtlGetVersion(&SystemInfo)));
	ASSERT(SystemInfo.dwMajorVersion == (ULONG)0x0000000A); // Windows 10
	ASSERT(SystemInfo.dwBuildNumber  == (ULONG)0x00004A64); // 19044

	// Local stack variables
	NTSTATUS       Status       = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = nullptr;
	
	// Register device driver and callbacks
	do {

		// Create the device object
		Status = ::WdmlibIoCreateDeviceSecure(
			DriverObject,
			0x00,
			&MppGlobals::DeviceName,
			FILE_DEVICE_UNKNOWN,
			0x00,
			TRUE,
			&SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
			&MppGlobals::DeviceId,
			&DeviceObject
		);
		if (NT_ERROR(Status) || DeviceObject == nullptr) {
			MppLogger::Error("Failed to create device object (0x%08X).\r\n", Status);
			break;
		}
		DriverObject->Flags |= DO_DIRECT_IO;

		// Create the symbolic link
		Status = ::IoCreateSymbolicLink(
			&MppGlobals::SymlinkName,
			&MppGlobals::DeviceName
		);
		if (NT_ERROR(Status)) {
			MppLogger::Error("Failed to symbolic link (0x%08X).\r\n", Status);
			break;
		}

		// Register callback for loaded modules
		Status = ::PsSetLoadImageNotifyRoutine(MppCallbacks::LoadImageNotify);
		if (NT_ERROR(Status)) {
			MppLogger::Error("Failed to register load image callback (0x%08X).\r\n", Status);
			break;
		}
		MppCallbacks::LoadImage = TRUE;

		MppWorker::ProtectWorker = ::IoAllocateWorkItem(DeviceObject);
		if (MppWorker::ProtectWorker == nullptr) {
			MppLogger::Error("Failed to allocate work item (0x%08X).\r\n", Status);
			break;
		}
		break;
	} while (true);

	// Check for errors
	if (NT_ERROR(Status)) {
		MppLogger::Info("=================================================================\r\n");
		MppLogger::Info("Rolling back operations ...\r\n");

		if (MppWorker::ProtectWorker != nullptr)
			::IoFreeWorkItem(MppWorker::ProtectWorker);

		if (MppCallbacks::LoadImage == TRUE)
			::PsRemoveLoadImageNotifyRoutine(MppCallbacks::LoadImageNotify);

		::IoDeleteSymbolicLink(&MppGlobals::SymlinkName);
		::IoDeleteDevice(DeviceObject);
		MppLogger::Info("=================================================================\r\n");
		return Status;
	}

	// Set driver's routines
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CREATE]         = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;

	// Set the IOCTL handler

	// Initialise additional data
	::KeInitializeSpinLock(&MppCallbackData::ImageNamesLock);
	::InitializeListHead(&MppCallbackData::HeadImageNames);

	// Get kernel base address
	Status = MppKernelRoutines::GetNtKernelBase();
	if (NT_ERROR(Status)) {
		MppLogger::Info("Failed to get NT kernel base address (0x%08X).\r\n", Status);
		return Status;
	}

	// Display final information
	MppLogger::Info("NT Kernel Address : 0x%p\r\n", MppKernelRoutines::KernelBase);
	MppLogger::Info("Device name       : %wZ\r\n",  MppGlobals::DeviceName);
	MppLogger::Info("Symbolic link name: %wZ\r\n",  MppGlobals::SymlinkName);
	MppLogger::Info("=================================================================\r\n");
	return Status;
}


_Use_decl_annotations_
EXTERN_C VOID DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
) {
	MppLogger::Info("=================================================================\r\n");

	// Free work item
	if (MppWorker::ProtectWorker != nullptr)
		::IoFreeWorkItem(MppWorker::ProtectWorker);

	// Remove load image callback
	::PsRemoveLoadImageNotifyRoutine(MppCallbacks::LoadImageNotify);
	MppCallbacks::LoadImage = FALSE;

	// Delete the symbolic link and the device object
	::IoDeleteSymbolicLink(&MppGlobals::SymlinkName);
	::IoDeleteDevice(DriverObject->DeviceObject);
}


_Use_decl_annotations_
EXTERN_C NTSTATUS DriverCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Status      = Status;
	Irp->IoStatus.Information = 0x00;
	
	::IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}


_Use_decl_annotations_
EXTERN_C NTSTATUS DriverDispatch(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	// Get the IRP Stack
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

	// Check if valid IOCTL has been sent
	NTSTATUS  Status = STATUS_SUCCESS;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode) {
	case MppIoctl::MppAddImageName(): {
		// Make sure input buffer is large enough
		if (!MppMemory::CheckInputBuffer(Stack, sizeof(MppIoctl::MPP_ADD_IMAGE_NAME))) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// Get input buffer
		MppIoctl::MPP_ADD_IMAGE_NAME InputBuffer = { 0x00 };
		__try {
			RtlCopyMemory(
				&InputBuffer,
				Irp->AssociatedIrp.SystemBuffer,
				sizeof(MppIoctl::MPP_ADD_IMAGE_NAME)
			);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			MppLogger::Error("Unreadable user-mode buffer.\r\n");
			Status = STATUS_ACCESS_VIOLATION;
			break;
		}

		// Add entry
		Status = MppCallbackData::AddImageName(
			InputBuffer.Name,
			InputBuffer.Size
		);
		break;
	}
	case MppIoctl::MppRemoveImageName(): {
		// Make sure input buffer is large enough
		if (!MppMemory::CheckInputBuffer(Stack, sizeof(MppIoctl::MPP_REMOVE_IMAGE_NAME))) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// Get input buffer
		MppIoctl::MPP_REMOVE_IMAGE_NAME InputBuffer = { 0x00 };
		__try {
			RtlCopyMemory(
				&InputBuffer,
				Irp->AssociatedIrp.SystemBuffer,
				sizeof(MppIoctl::MPP_REMOVE_IMAGE_NAME)
			);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			MppLogger::Error("Unreadable user-mode buffer.\r\n");
			Status = STATUS_ACCESS_VIOLATION;
			break;
		}

		// Remove entry
		Status = MppCallbackData::RemoveImageName(InputBuffer.Name);
		break;
	}
	case MppIoctl::MppInitialiseKernelRoutines(): {

		// Check if routines have already been initialised
		if (MppKernelRoutines::RoutinesInitialised == TRUE)
			break;

		// Make sure input buffer is large enough
		if (!MppMemory::CheckInputBuffer(Stack, sizeof(MppIoctl::MPP_KERNEL_ROUTINES_OFFSETS))) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// Get input buffer
		MppIoctl::MPP_KERNEL_ROUTINES_OFFSETS InputBuffer = { 0x00 };
		__try {
			RtlCopyMemory(
				&InputBuffer,
				Irp->AssociatedIrp.SystemBuffer,
				sizeof(MppIoctl::MPP_KERNEL_ROUTINES_OFFSETS)
			);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			MppLogger::Error("Unreadable user-mode buffer.\r\n");
			Status = STATUS_ACCESS_VIOLATION;
			break;
		}

		// Update function pointers
		MppKernelRoutines::UpdateFunctionPointer(
			MppKernelRoutines::MiAddSecureEntry,
			InputBuffer.MiAddSecureEntry
		);
		MppKernelRoutines::UpdateFunctionPointer(
			MppKernelRoutines::MiObtainReferencedVadEx,
			InputBuffer.MiObtainReferencedVadEx
		);
		MppKernelRoutines::UpdateFunctionPointer(
			MppKernelRoutines::MiUnlockAndDereferenceVad,
			InputBuffer.MiUnlockAndDereferenceVad
		);

		MppKernelRoutines::RoutinesInitialised = TRUE;
		break;
	}
	default:
		MppLogger::Info("Invalid IOCTL: 0x%08x\r\n", Stack->Parameters.DeviceIoControl.IoControlCode);
		Status = STATUS_INVALID_DEVICE_REQUEST;
	}


	// Complete request
	Irp->IoStatus.Status      = Status;
	Irp->IoStatus.Information = 0x00;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}