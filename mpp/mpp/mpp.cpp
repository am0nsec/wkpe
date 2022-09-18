/*+================================================================================================
Module Name: mpp.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Memory Patching Protection (MPP) Global Kernel Data.
================================================================================================+*/

#include "mpp.hpp"
#include "worker.hpp"


/// @brief MPP Global variables
namespace MppGlobals {
	/// @brief Kernel memory pool tag.
	ULONG PoolTag = (ULONG)' ppM';

	/// @brief Name of the kernel device driver.
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MppDrv");

	/// @brief Symbolic link name pointing to kernel device driver.
	UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\MppDrv");

	/// @brief Unique Identifier for the kernel device driver - {F394F785-1D05-4C02-A3C5-6E3DC8F0BAD9}.
	CONST GUID DeviceId = {
		0xf394f785,
		0x1d05,
		0x4c02,
		{ 0xa3, 0xc5, 0x6e, 0x3d, 0xc8, 0xf0, 0xba, 0xd9 }
	};

	/// @brief Work item to protect .text section of a module.
	PIO_WORKITEM ProtectWorker = nullptr;
}


/// @brief Routines and data related to callbacks.
namespace MppCallbacks {

	/// @brief Weather the LoadImageNotify callbacks has been set.
	 BOOLEAN LoadImage = FALSE;
}


/// @brief Data used by the various callbacks
namespace MppCallbackData {
	/// @brief Double-linked list of image names to protect .text section.
	LIST_ENTRY HeadImageNames{};

	/// @brief Number of image names to check for each modules being loaded.
	ULONG      NumberOfImageNames = 0x00;

	/// @brief Spin lock for double-linked list of image names.
	KSPIN_LOCK ImageNamesLock = 0x00;
}


/// @brief List of non-exported kernel routines required to secure .text section of a module.
namespace MppKernelRoutines {
	PVOID(STDMETHODCALLTYPE* MiObtainReferencedVadEx)(
		_In_  PVOID     Address,
		_In_  UCHAR     Flags,
		_Out_ NTSTATUS* Status
	) = nullptr;

	VOID(STDMETHODCALLTYPE* MiUnlockAndDereferenceVad)(
		_In_  PVOID VadObject
	) = nullptr;

	PVOID(STDMETHODCALLTYPE* MiAddSecureEntry) (
		_In_ PVOID VadObject,
		_In_ PVOID AddressStart,
		_In_ PVOID AddressEnd,
		_In_ ULONG ProbMode,
		_In_ ULONG Flags
	) = nullptr;

	/// @brief Base address of NT kernel image.
	UINT64 KernelBase = 0x00;

	/// @brief Whether the kernel routines have been initialised.
	BOOLEAN RoutinesInitialised = FALSE;
}


_Use_decl_annotations_
VOID __declspec(code_seg("PAGE"))
MppCallbacks::LoadImageNotify(
	_In_opt_ PUNICODE_STRING FullImageName,
	_In_     HANDLE          ProcessId,
	_In_     PIMAGE_INFO     ImageInfo
) {
	UNREFERENCED_PARAMETER(ImageInfo);

	// Cannot check if there is no name
	if (FullImageName == nullptr)
		return;

	// Acquire spin-lock
	KLOCK_QUEUE_HANDLE LockHandle = { 0x00 };
	::KeAcquireInStackQueuedSpinLock(
		&MppCallbackData::ImageNamesLock,
		&LockHandle
	);

	// Check if name is of our interest
	BOOLEAN     Found = FALSE;
	PLIST_ENTRY Head  = MppCallbackData::HeadImageNames.Flink;

	for (ULONG cx = 0x00; cx < MppCallbackData::NumberOfImageNames; cx++) {
		auto Entry = CONTAINING_RECORD(Head, MppCallbackData::ImageNameEntry, List);
		if (Entry == nullptr)
			break;

		// Check if name matches
		if (::wcsstr(FullImageName->Buffer, Entry->Name) != NULL) {
			Found = TRUE;
			break;
		}

		if (Head->Flink == &MppCallbackData::HeadImageNames)
			break;
		Head = Head->Flink;
	}

	// Release spin-lock
	::KeReleaseInStackQueuedSpinLock(&LockHandle);

	// Check if found
	if (!Found)
		return;

	// Get information required
	MppLogger::Info("Image      : %wZ\r\n", FullImageName);

	// Allocate memory for worker context data
	auto WorkerData = MppMemory::MemAlloc<MppWorker::MPP_WORKER_PROTECT_DATA*>(
		sizeof(MppWorker::MPP_WORKER_PROTECT_DATA)
	);
	if (WorkerData == nullptr) {
		MppLogger::Error("Unable to allocate memory for protect worker data\r\n");
		return;
	}

	// Add information
	WorkerData->ThreadId         = ::PsGetCurrentThreadId();
	WorkerData->ProcessId        = ProcessId;
	WorkerData->ImageBaseAddress = ImageInfo->ImageBase;

	// Fire the work item from kernel thread pool
	::IoQueueWorkItem(
		MppWorker::ProtectWorker,
		(PIO_WORKITEM_ROUTINE)MppWorker::ProtectWorkerCallback,
		DelayedWorkQueue,
		WorkerData
	);
}


_Use_decl_annotations_
HRESULT __declspec(code_seg("PAGE"))
MppCallbackData::AddImageName(
	_In_  LPWSTR  ImageName,
	_In_  SIZE_T  ImageNameSize
) {
	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check for parameters
	if (ImageName == nullptr)
		return STATUS_INVALID_PARAMETER_1;
	if (ImageNameSize == 0x00)
		return STATUS_INVALID_PARAMETER_2;

	// Allocate memory
	auto Entry = MppMemory::MemAlloc<ImageNameEntry*>(sizeof(ImageNameEntry));
	if (Entry == nullptr)
		return STATUS_NO_MEMORY;

	// Move data to 
	Entry->Size = ImageNameSize;
	RtlCopyMemory(Entry->Name, ImageName, ImageNameSize);

	// Acquire spin-lock
	KLOCK_QUEUE_HANDLE LockHandle = { 0x00 };
	::KeAcquireInStackQueuedSpinLock(
		&ImageNamesLock,
		&LockHandle
	);

	// Add entry to the end of the list
	::InsertTailList(
		&HeadImageNames,
		reinterpret_cast<PLIST_ENTRY>(Entry)
	);
	NumberOfImageNames++;

	// Release spin-lock
	::KeReleaseInStackQueuedSpinLock(&LockHandle);

	// Return success
	return STATUS_SUCCESS;
}


_Use_decl_annotations_
HRESULT __declspec(code_seg("PAGE"))
MppCallbackData::RemoveImageName(
	_In_ LPWSTR ImageName
) {
	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check for parameter
	if (ImageName == nullptr)
		return STATUS_INVALID_PARAMETER_1;

	// Acquire spin-lock
	KLOCK_QUEUE_HANDLE LockHandle = { 0x00 };
	::KeAcquireInStackQueuedSpinLock(
		&ImageNamesLock,
		&LockHandle
	);
	
	// Parse all entries
	ImageNameEntry* Entry = nullptr;
	PLIST_ENTRY     Head  = HeadImageNames.Flink;
	BOOLEAN         Found = FALSE;
	for (UINT32 cx = 0x00; cx < NumberOfImageNames; cx++) {

		// Structure from memory
		Entry = CONTAINING_RECORD(Head, ImageNameEntry, List);
		if (Entry == nullptr)
			break;

		// Check if name matches
		if (::wcsstr(ImageName, Entry->Name) != NULL) {
			Found = TRUE;
			break;
		}

		// Move to next entry
		if (Head->Flink == &HeadImageNames)
			break;
		Head = Head->Flink;
	}

	// Remove if entry was found
	if (Found) {
		::RemoveEntryList(reinterpret_cast<PLIST_ENTRY>(Entry));
		MppMemory::MemFree(Entry);
	}

	// Release spin-lock
	::KeReleaseInStackQueuedSpinLock(&LockHandle);

	// Return success
	return STATUS_SUCCESS;
}


_Use_decl_annotations_
HRESULT __declspec(code_seg("PAGE"))
MppKernelRoutines::GetNtKernelBase() {
	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Initialise auxiliary kernel library
	::AuxKlibInitialize();

	ULONG BufferSize = 0x00;
	NTSTATUS Status = ::AuxKlibQueryModuleInformation(&BufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
	if (NT_ERROR(Status) || BufferSize == 0x00)
		return STATUS_UNSUCCESSFUL;

	auto ExtendedInfo = MppMemory::MemAlloc<PAUX_MODULE_EXTENDED_INFO>(BufferSize);
	if (ExtendedInfo == NULL)
		return STATUS_NO_MEMORY;
	Status = ::AuxKlibQueryModuleInformation(&BufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), (PVOID)ExtendedInfo);

	// Parse all modules
	for (UINT32 cx = 0x00; cx < (BufferSize / sizeof(AUX_MODULE_EXTENDED_INFO)); cx++) {
		if (::strcmp("\\SystemRoot\\system32\\ntoskrnl.exe", (CONST PCHAR)ExtendedInfo[cx].FullPathName) == 0x00) {
			MppKernelRoutines::KernelBase = (UINT64)ExtendedInfo[cx].BasicInfo.ImageBase;
			break;
		}
	}
	ASSERT(MppKernelRoutines::KernelBase != 0x00);

	// Cleanup and return
	MppMemory::MemFree(ExtendedInfo);
	return STATUS_SUCCESS;
}