/*+================================================================================================
Module Name: mmanager-routines.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
Handle User-Mode IOCTL requests.

================================================================================================+*/

#include "mmanager-routines.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MmanIoctlFindProcessVads)
#pragma alloc_text(PAGE, MmanIoctlGetProcessVads)

#pragma alloc_text(PAGE, MmanpCalculateStructureSize)
#endif // ALLOC_PRAGMA

// Global variable to store the size of the UM structure.
ULONG64 VadTableSize = 0x00;

// Global variable to build the UM structure.
XVAD_TABLE VadTable = {0x00 };


_Use_decl_annotations_
EXTERN_C NTSTATUS MmanIoctlFindProcessVads(
	_In_ PIRP               Irp,
	_In_ PIO_STACK_LOCATION Stack
) {
	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check for the input length
	if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
		return STATUS_BUFFER_TOO_SMALL;

	// Check for output length
	if (MmGetMdlByteCount(Irp->MdlAddress) < sizeof(ULONG64)) {
		MMDebug(("MDL too small.\r\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	PVOID UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (UserBuffer == NULL) {
		MMDebug(("Unable to get MDL.\r\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Filter out the system "process"
	ULONG ProcessId = 0x00;
	__try {
		RtlCopyMemory(&ProcessId, Irp->AssociatedIrp.SystemBuffer, sizeof(ULONG));
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		MMDebug(("Unreadable user-mode buffer.\r\n"));
		return STATUS_ACCESS_VIOLATION;
	}
	if (ProcessId <= 0x04 || (ProcessId % 0x04) != 0x00) {
		MMDebug(("Invalid process ID supplied.\r\n"));
		return STATUS_INVALID_PARAMETER;
	}
	MMDebug(("Process ID       : 0x%x\r\n", ProcessId));

	// Forward declaration of stack variables
	NTSTATUS   Status          = STATUS_SUCCESS;
	BOOLEAN    ProcessAttached = FALSE;
	KAPC_STATE ProcessApcState = { 0x00 };

	// Get the EPROCESS structure based on the requested ID
	PEPROCESS Process = NULL;
	Status = PsLookupProcessByProcessId(ULongToHandle(ProcessId), &Process);
	if (!NT_SUCCESS(Status)) {
		MMDebug(("Unable to get the _EPROCESS structure for the given PID (0x%08x).\r\n", Status));
		goto exit;
	}

	// Increase reference counter to prevent process from terminating while 
	// parsing the VAD tree
	ObReferenceObject(Process);
	KeStackAttachProcess(Process, &ProcessApcState);
	ProcessAttached = TRUE;
	MMDebug(("_EPROCESS address: 0x%p\r\n", Process));

	// Initialize internal VAD table
	Status = XMiInitializeVadTable(Process, &VadTable);
	if (!NT_SUCCESS(Status)) {
		MMDebug(("Failed to initialize the VAD table (0x%08x).\r\n", Status));
		goto exit;
	}

	// Get the whole table
	VadTable.Process = Process;
	XMiBuildVadTable(&VadTable, NULL, NULL, 0x00);
	
	// Release the proces object
	KeUnstackDetachProcess(&ProcessApcState);
	ObDereferenceObject(Process);
	ProcessAttached = FALSE;

	// Calculate the user-mode size of the data
	VadTableSize = MmanpCalculateStructureSize();

	// Return the data to the user-mode caller
	__try {
		RtlCopyMemory(UserBuffer, &VadTableSize, sizeof(ULONG64));
		return STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		MMDebug(("Unable to return buffer size to caller.\r\n"));
		Status = STATUS_INVALID_PARAMETER;
	}

	// Cleanup and dereference the object
exit:
	if (ProcessAttached)
		KeUnstackDetachProcess(&ProcessApcState);
	if (Process != NULL)
		ObDereferenceObject(Process);
	if (VadTable.Process == NULL) {
		VadTableSize = 0x00;
		XMiUninitializeVadTable(&VadTable);
	}
	return Status;
}


_Use_decl_annotations_
EXTERN_C NTSTATUS MmanIoctlGetProcessVads(
	_In_  PIRP               Irp,
	_In_  PIO_STACK_LOCATION Stack,
	_Out_ ULONG_PTR*         BufferOutSize
) {
	UNREFERENCED_PARAMETER(Stack);

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	NTSTATUS Status     = STATUS_SUCCESS;
	PVOID    UserBuffer = NULL;
	ULONG64  UmAddress  = 0x00;

	// Check the input buffer
	if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG64)) {
		MMDebug(("Buffer too small.\r\n"));
		Status = STATUS_BUFFER_TOO_SMALL;
		goto exit;
	}
	__try {
		RtlCopyMemory(&UmAddress, Irp->AssociatedIrp.SystemBuffer, sizeof(ULONG64));
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		MMDebug(("Unreadable user-mode buffer.\r\n"));
		Status = STATUS_ACCESS_VIOLATION;
		goto exit;
	}

	// Check the size of the output buffer
	if (MmGetMdlByteCount(Irp->MdlAddress) < VadTableSize) {
		MMDebug(("MDL too small.\r\n"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}
	UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (UserBuffer == NULL) {
		MMDebug(("Unable to get MDL.\r\n"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	// Get the header 
	PMMANAGER_VADLIST_HEADER Header = UserBuffer;
	Header->Size = sizeof(MMANAGER_VADLIST_HEADER);
	Header->MaximumLevel = VadTable.MaximumLevel;
	Header->NumberOfNodes = VadTable.NumberOfNodes;
	Header->TotalPrivateCommit = VadTable.TotalPrivateCommit;
	Header->TotalSharedCommit = VadTable.TotalSharedCommit;
	Header->Eprocess = VadTable.Process;

	// Get address of first entry
	PVOID EntryPoint = (PUCHAR)Header + Header->Size;
	Header->First = XLATE_TO_UM_ADDRESS(UmAddress, Header, EntryPoint);

	PMMANAGER_VADLIST_ENTRY Blink = NULL;

	// Parse all entries
	PLIST_ENTRY ListEntry = VadTable.InsertOrderList.Flink;
	do {
		PXVAD_TABLE_ENTRY TableEntry = CONTAINING_RECORD(ListEntry, XVAD_TABLE_ENTRY, List);
		if (TableEntry == NULL)
			break;

		// New entry in output memory
		PMMANAGER_VADLIST_ENTRY OutEntry = EntryPoint;
		OutEntry->List.Blink = XLATE_TO_UM_ADDRESS(UmAddress, Header, Blink);

		if (OutEntry == Header->First)
			OutEntry->List.Blink = NULL;

		OutEntry->VadAddress    = TableEntry->Address;
		OutEntry->Level         = TableEntry->Level;
		OutEntry->VpnStarting   = TableEntry->StartingVpn;
		OutEntry->VpnEnding     = TableEntry->EndingVpn;
		OutEntry->CommitCharge  = TableEntry->CommitCharge;
		OutEntry->LongVadFlags  = TableEntry->LongVadFlags;
		OutEntry->LongVadFlags1 = TableEntry->LongVadFlags1;
		OutEntry->LongVadFlags2 = TableEntry->LongVadFlags2;

		if (TableEntry->Name != NULL) {
			OutEntry->FileNameSize = TableEntry->Name->Length;
			RtlCopyMemory(OutEntry->FileName, TableEntry->Name->Buffer, OutEntry->FileNameSize);
		}
		else if (TableEntry->ControlArea != NULL) {
			OutEntry->CommitPageCount = TableEntry->ControlArea->u3.CommittedPageCount;
		}

		OutEntry->Size = sizeof(MMANAGER_VADLIST_ENTRY) + OutEntry->FileNameSize;
		Header->Size += OutEntry->Size;

		// Check for end of the parsing
		if (ListEntry->Flink == &VadTable.InsertOrderList) {
			OutEntry->List.Flink = NULL;
			break;
		}

		// Move to next entry
		EntryPoint = (PUCHAR)EntryPoint + OutEntry->Size;
		OutEntry->List.Flink = XLATE_TO_UM_ADDRESS(UmAddress, Header, EntryPoint);

		Blink = OutEntry;
		ListEntry = ListEntry->Flink;
	} while (TRUE);

	// Set last data
	Header->Last = XLATE_TO_UM_ADDRESS(UmAddress, Header, EntryPoint);;
	*BufferOutSize = Header->Size;
exit:
	// Release memory
	if (VadTableSize != 0x00) {
		XMiUninitializeVadTable(&VadTable);
		VadTableSize = 0x00;
	}
	return Status;
}


_Use_decl_annotations_
EXTERN_C ULONG64 MmanpCalculateStructureSize(
	VOID
) {
	if (VadTable.Process == NULL)
		return 0x00;

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	ULONG64 TotalSize = sizeof(MMANAGER_VADLIST_HEADER);

	// Parse all entries
	PLIST_ENTRY Entry = VadTable.InsertOrderList.Flink;
	do {
		PXVAD_TABLE_ENTRY TableEntry = CONTAINING_RECORD(Entry, XVAD_TABLE_ENTRY, List);
		if (TableEntry == NULL)
			break;

		TotalSize += sizeof(MMANAGER_VADLIST_ENTRY);
		if (TableEntry->Name != NULL)
			TotalSize += TableEntry->Name->Length + sizeof(WCHAR);

		// Next entry
		if (Entry->Flink == &VadTable.InsertOrderList)
			break;
		Entry = Entry->Flink;
	} while (TRUE);

	// Return the aligned memory.
	while ((TotalSize % PAGE_SIZE) != 0x00)
		TotalSize++;
	return TotalSize;
}