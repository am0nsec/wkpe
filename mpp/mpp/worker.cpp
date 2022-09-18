/*+================================================================================================
Module Name: worker.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Memory Patching Protection (MPP) Worker for Image .text section protection.
================================================================================================+*/

#include "mpp.hpp"
#include "worker.hpp"


namespace MppWorker {

	/// @brief Work item to protect .text section of a module.
	PIO_WORKITEM ProtectWorker = nullptr;
}


_Use_decl_annotations_
VOID __declspec(code_seg("PAGE"))
MppWorker::ProtectWorkerCallback(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_opt_ PVOID          WorkerData
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	if (WorkerData == nullptr)
		return;
	
	// Get local stack copy of the worker data
	MppWorker::MPP_WORKER_PROTECT_DATA LocalWorkerData = { 0x00 };
	RtlCopyMemory(
		&LocalWorkerData,
		WorkerData,
		sizeof(MppWorker::MPP_WORKER_PROTECT_DATA)
	);
	MppMemory::MemFree(WorkerData);
	
	// Log the information sent via from the callback
	MppLogger::Info("Process ID : 0x%08x\r\n", ::HandleToULong(LocalWorkerData.ProcessId));
	MppLogger::Info("Thread  ID : 0x%08x\r\n", ::HandleToULong(LocalWorkerData.ThreadId));
	MppLogger::Info("Image      : 0x%p\r\n",   LocalWorkerData.ImageBaseAddress);

	// Attack to process
	PEPROCESS  TargetProcess  = NULL;
	KAPC_STATE ApcState       = { 0x00 };

	NTSTATUS Status = ::PsLookupProcessByProcessId(LocalWorkerData.ProcessId, &TargetProcess);
	if (!NT_SUCCESS(Status)) {
		MppLogger::Error("Unable to get PEPROCESS from PID (0x%08X).\r\n", Status);
		return;
	}
	::KeStackAttachProcess(TargetProcess, &ApcState);

	// Get the start and end address of the .text section of the image
	PVOID AddressStart = nullptr;
	PVOID AddressEnd   = nullptr;

	MppWorker::GetImageTextSectionAddresses(
		LocalWorkerData.ImageBaseAddress,
		&AddressStart,
		&AddressEnd
	);
	if (AddressStart == nullptr || AddressEnd == nullptr) {
		::KeUnstackDetachProcess(&ApcState);

		MppLogger::Error("Unable get the start and end address of the .text section.\r\n");
		return;
	}
	MppLogger::Info(".text start: 0x%p\r\n", AddressStart);
	MppLogger::Info(".text end  : 0x%p\r\n", AddressEnd);

	// Raise IRQL to APC_LEVEL (1)
	KIRQL OldIRQL = ::KfRaiseIrql(APC_LEVEL);

	// Get a reference to the Virtual Address Descriptor (VAD)
	PVOID VadObject = MppKernelRoutines::MiObtainReferencedVadEx(
		AddressStart,
		0x00,
		&Status
	);
	if (!NT_SUCCESS(Status)) {
		MppLogger::Error("Unable to get VAD for address (0x%08X).\r\n", Status);

		::KeLowerIrql(OldIRQL);
		::KeUnstackDetachProcess(&ApcState);
		return;
	}

	// Enforce non-writable and non-modifiable memory pages
	PVOID PoolMm = MppKernelRoutines::MiAddSecureEntry(
		VadObject,
		AddressStart,
		AddressEnd,
		(PAGE_REVERT_TO_FILE_MAP | PAGE_TARGETS_NO_UPDATE | PAGE_NOACCESS),
		(MM_SECURE_USER_MODE_ONLY | MM_SECURE_NO_CHANGE)
	);
	if (!(PoolMm >= 0x00)) {
		MppLogger::Error("Unable to secure image (0x%08X).\r\n", Status);
	}

	// Cleanup
	MppKernelRoutines::MiUnlockAndDereferenceVad(VadObject);

	::KeLowerIrql(OldIRQL);
	::KeUnstackDetachProcess(&ApcState);
	return;
}


_Use_decl_annotations_
VOID __declspec(code_seg("PAGE"))
MppWorker::GetImageTextSectionAddresses(
	_In_  PVOID  ImageBaseAddress,
	_Out_ PVOID* AddressStart,
	_Out_ PVOID* AddressEnd
) {
	if (ImageBaseAddress == NULL)
		return;
	*AddressStart = 0x00;
	*AddressEnd   = 0x00;

	PIMAGE_DOS_HEADER ImageDOSHeader = (PIMAGE_DOS_HEADER)ImageBaseAddress;
	if (ImageDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		MppLogger::Error("Invalid DOS signature.\r\n");
		return;
	}

	PIMAGE_NT_HEADERS ImageNTHeaders = (PIMAGE_NT_HEADERS)((PUCHAR)ImageBaseAddress + ImageDOSHeader->e_lfanew);
	if (ImageNTHeaders->Signature != IMAGE_NT_SIGNATURE) {
		MppLogger::Error("Invalid NT Headers signature.\r\n");
		return;
	}

	PIMAGE_SECTION_HEADER SectionHeader = IMAGE_FIRST_SECTION(ImageNTHeaders);
	while (TRUE) {
		if (MppWorker::IsTextSection(SectionHeader->Name))
			break;
		SectionHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)SectionHeader + IMAGE_SIZEOF_SECTION_HEADER);
	}

	*AddressStart = (PVOID)(((ULONG_PTR)ImageBaseAddress  + SectionHeader->VirtualAddress) & 0xFFFFFFFFFFFFF000);
	*AddressEnd   = (PVOID)((ULONG_PTR)*AddressStart + SectionHeader->SizeOfRawData);
	return;
}
