/*+================================================================================================
Module Name: wki.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wspe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Kernel Introspection (WKI).

================================================================================================+*/

#include "wki.h"

// Global Windows Kernel Introspection (WKI) Data.
WKI_GLOBALS WkiGlobal = { 0x00 };

// Global boolean flag
BOOLEAN WkiInitialised = FALSE;


EXTERN_C NTSTATUS
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
WkipGetInitialRegistryKey(
	_Out_ PHANDLE RegistryKey
);


EXTERN_C NTSTATUS
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
WkipGetSymbolEntries(
	_In_ CONST HANDLE RegistryKey
);


EXTERN_C NTSTATUS
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
WkipGetSystemImageBase(
	VOID
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WkiGetSymbol)
#pragma alloc_text(PAGE, WkiReadValue)
#pragma alloc_text(PAGE, WkiInitialise)
#pragma alloc_text(PAGE, WkiUninitialise)

#pragma alloc_text(PAGE, WkipGetSymbolEntries)
#pragma alloc_text(PAGE, WkipGetSystemImageBase)
#pragma alloc_text(PAGE, WkipGetInitialRegistryKey)
#endif // ALLOC_PRAGMA


_Use_decl_annotations_
EXTERN_C NTSTATUS WkipGetInitialRegistryKey(
	_Out_ PHANDLE RegistryKey
) {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	if (RegistryKey == NULL)
		return STATUS_INVALID_PARAMETER_1;
	*RegistryKey = NULL;

	// Open Registry key to the current OS version
	UNICODE_STRING    ObjName       = RTL_CONSTANT_STRING(WKI_CURRENTVERSION_KEY_NAME);
	OBJECT_ATTRIBUTES ObjAttributes = { 0x00 };
	InitializeObjectAttributes(&ObjAttributes, &ObjName, (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE), 0x00, 0x00);
	
	HANDLE CurrentVersion = NULL;
	if (NT_ERROR(ZwOpenKey(&CurrentVersion, GENERIC_READ, &ObjAttributes)))
		return STATUS_UNSUCCESSFUL;

	// Values to parse
	LPCWSTR OsVersionValues[] = {
		L"CurrentMajorVersionNumber",
		L"CurrentBuildNumber",
		L"UBR"
	};

	DWORD Major                    = 0x00;
	DWORD Revision                 = 0x00;
	WCHAR CurrentBuildNumber[0x08] = { 0x00 };

	// Get the values required
	for (UINT16 cx = 0x00; cx < _ARRAYSIZE(OsVersionValues); cx++) {

		// Initialise value name
		UNICODE_STRING ValueName = { 0x00 };
		RtlUnicodeStringInit(&ValueName, OsVersionValues[cx]);

		// Get the size of the buffer to be returned
		ULONG PartialInfoSize = 0x00;
		NTSTATUS Status = ZwQueryValueKey(CurrentVersion, &ValueName, KeyValuePartialInformation, NULL, 0x00, &PartialInfoSize);
		if (Status != STATUS_BUFFER_TOO_SMALL) {
			ZwClose(CurrentVersion);
			return STATUS_UNSUCCESSFUL;
		}

		// Allocate memory for value
		PKEY_VALUE_PARTIAL_INFORMATION PartialInformation = ExAllocatePool2(POOL_FLAG_PAGED, (SIZE_T)PartialInfoSize, WKI_MM_TAG);
		if (PartialInformation == NULL) {
			ZwClose(CurrentVersion);
			return STATUS_NO_MEMORY;
		}

		// Get the partial information
		Status = ZwQueryValueKey(CurrentVersion, &ValueName, KeyValuePartialInformation, (PVOID)PartialInformation, PartialInfoSize, &PartialInfoSize);
		if (NT_ERROR(Status)) {
			ExFreePoolWithTag((PVOID)PartialInfoSize, WKI_MM_TAG);
			ZwClose(CurrentVersion);
			return STATUS_UNSUCCESSFUL;
		}

		// Copy the value
		if (cx == 0x00)
			RtlCopyMemory(&Major, PartialInformation->Data, PartialInformation->DataLength);
		else if (cx == 0x01)
			RtlCopyMemory(CurrentBuildNumber, PartialInformation->Data, PartialInformation->DataLength);
		else if (cx == 0x02)
			RtlCopyMemory(&Revision, PartialInformation->Data, PartialInformation->DataLength);

		// Free memory
		ExFreePoolWithTag((PVOID)PartialInformation, WKI_MM_TAG);
	}
	ZwClose(CurrentVersion);
	CurrentVersion = NULL;

	// Keys to parse to get the initial Kernel Introspection key

	UNICODE_STRING    WkiKeyName       = RTL_CONSTANT_STRING(WKI_KINTROSPECTION_KEY_NAME);
	OBJECT_ATTRIBUTES WkiKeyAttributes = { 0x00 };
	InitializeObjectAttributes(&WkiKeyAttributes, &WkiKeyName, (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE), 0x00, 0x00);

	HANDLE WkiKeyBase = NULL;
	if (NT_ERROR(ZwOpenKey(&WkiKeyBase, GENERIC_READ, &WkiKeyAttributes)))
		return STATUS_UNSUCCESSFUL;

	// Build the string
	SIZE_T BufferSize = (sizeof(DWORD) * 4) + (sizeof(WCHAR) * 0x0A);
	LPWSTR Buffer     = ExAllocatePool2(POOL_FLAG_PAGED, BufferSize, WKI_MM_TAG);
	if (Buffer == NULL)
		return STATUS_NO_MEMORY;
	swprintf_s(Buffer, BufferSize / sizeof(WCHAR), L"%u.%s.%u", Major, CurrentBuildNumber, Revision);

	// Open handle to the key associated with the current OS version.
	UNICODE_STRING OSVersion = { 0x00 };
	RtlUnicodeStringInit(&OSVersion, Buffer);

	OBJECT_ATTRIBUTES ObjectAttributes = { 0x00 };
	InitializeObjectAttributes(&ObjectAttributes, &OSVersion, OBJ_CASE_INSENSITIVE, WkiKeyBase, 0x00);

	NTSTATUS Status = STATUS_SUCCESS;
	Status = ZwOpenKey(RegistryKey, (GENERIC_READ), &ObjectAttributes);

	// Cleanup and exit
	ZwClose(WkiKeyBase);
	ExFreePoolWithTag((PVOID)Buffer, WKI_MM_TAG);
	return STATUS_SUCCESS;
}


_Use_decl_annotations_
EXTERN_C NTSTATUS WkipGetSymbolEntries(
	_In_ CONST HANDLE RegistryKey
) {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	if (RegistryKey == NULL)
		return STATUS_INVALID_PARAMETER_1;

	// Open symbol key
	UNICODE_STRING    ObjectName       = RTL_CONSTANT_STRING(L"Symbols");
	OBJECT_ATTRIBUTES ObjectAttributes = { 0x00 };
	InitializeObjectAttributes(&ObjectAttributes, &ObjectName, OBJ_CASE_INSENSITIVE, RegistryKey, 0x00);

	HANDLE SymbolKey = NULL;
	if (NT_ERROR(ZwOpenKey(&SymbolKey, GENERIC_READ, &ObjectAttributes)))
		return STATUS_UNSUCCESSFUL;

	// Get all the sub-keys
	ULONG Index = 0x00;
	do {

		// Early declartion of local stack variables for goto usage
		NTSTATUS               Status           = STATUS_SUCCESS;
		PKEY_BASIC_INFORMATION BasicInfo        = NULL;

		UNICODE_STRING         SubKeyName       = { 0x00 };
		OBJECT_ATTRIBUTES      SubKeyAttributes = { 0x00 };
		HANDLE                 SubKey           = NULL;

		// Get size of the basic information
		ULONG BasicInfoSize = 0x00;
		Status = ZwEnumerateKey(SymbolKey, Index, KeyBasicInformation, NULL, 0x00, &BasicInfoSize);

		// Check first status code
		if (Status == STATUS_NO_MORE_ENTRIES)
			break;
		else if (Status != STATUS_BUFFER_TOO_SMALL)
			goto next_entry;

		// Allocate memory for the Basic Information
		BasicInfo = ExAllocatePool2(POOL_FLAG_PAGED, (SIZE_T)BasicInfoSize, WKI_MM_TAG);
		if (BasicInfo == NULL) {
			Status = STATUS_NO_MEMORY;
			goto next_entry;
		}

		// Get the basic information
		Status = ZwEnumerateKey(SymbolKey, Index, KeyBasicInformation, (PVOID)BasicInfo, BasicInfoSize, &BasicInfoSize);
		if (NT_ERROR(Status))
			goto next_entry;

		// Open sub-key
		RtlInitUnicodeString(&SubKeyName, BasicInfo->Name);
		InitializeObjectAttributes(&SubKeyAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE, SymbolKey, 0x00);

		if (NT_ERROR(ZwOpenKey(&SubKey, GENERIC_READ, &SubKeyAttributes)))
			goto next_entry;

		// Allocate memory for the symbol entry
		PWKI_SYMBOL_ENTRY SymbolEntry = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(WKI_SYMBOL_ENTRY), WKI_MM_TAG);
		if (SymbolEntry == NULL) {
			Status = STATUS_NO_MEMORY;
			goto next_entry;
		}

		// Parse all the values from the Sub-key
		LPCWSTR SymbolValues[] = {
			L"DJB",
			L"OFF",
			L"RVA",
			L"SEG"
		};
		for (UINT32 cx = 0x00; cx < _ARRAYSIZE(SymbolValues); cx++) {

			UNICODE_STRING ValueName = { 0x00 };
			RtlUnicodeStringInit(&ValueName, SymbolValues[cx]);

			// Get the size of the buffer to be returned
			ULONG PartialInfoSize = 0x00;
			Status = ZwQueryValueKey(SubKey, &ValueName, KeyValuePartialInformation, NULL, 0x00, &PartialInfoSize);
			if (Status != STATUS_BUFFER_TOO_SMALL) {
				ExFreePoolWithTag((PVOID)SymbolEntry, WKI_MM_TAG);
				break;
			}

			// Allocate memory for value
			PKEY_VALUE_PARTIAL_INFORMATION PartialInformation = ExAllocatePool2(POOL_FLAG_PAGED, (SIZE_T)PartialInfoSize, WKI_MM_TAG);
			if (PartialInformation == NULL) {
				ExFreePoolWithTag((PVOID)SymbolEntry, WKI_MM_TAG);
				break;
			}

			// Get the partial information
			Status = ZwQueryValueKey(SubKey, &ValueName, KeyValuePartialInformation, (PVOID)PartialInformation, PartialInfoSize, &PartialInfoSize);
			if (NT_ERROR(Status)) {
				ExFreePoolWithTag((PVOID)SymbolEntry, WKI_MM_TAG);
				ExFreePoolWithTag((PVOID)PartialInformation, WKI_MM_TAG);
				break;
			}

			// Copy the value
			PVOID Destination = NULL;
			if (cx == 0x00)
				Destination = &SymbolEntry->Body.DJB;
			else if (cx == 0x01)
				Destination = &SymbolEntry->Body.OFF;
			else if (cx == 0x02)
				Destination = &SymbolEntry->Body.RVA;
			else if (cx == 0x03)
				Destination = &SymbolEntry->Body.SEG;
			RtlCopyMemory(Destination, PartialInformation->Data, PartialInformation->DataLength);
			ExFreePoolWithTag((PVOID)PartialInformation, WKI_MM_TAG);
		}

		// Add new entry in the double-linked list
		if (NT_SUCCESS(Status) && SymbolEntry != NULL) {
			WkiGlobal.NumberOfSymbols++;
			InsertTailList(&WkiGlobal.SymbolHead, &SymbolEntry->List);
		}

		// Cleanup and next entry
	next_entry:
		if (BasicInfo != NULL)
			ExFreePoolWithTag((PVOID)BasicInfo, WKI_MM_TAG);

		Index++;
	} while (TRUE);

	// Cleanup
	ZwClose(SymbolKey);
	return STATUS_SUCCESS;
}


_Use_decl_annotations_
EXTERN_C NTSTATUS WkipGetSystemImageBase(
	VOID
) {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Initialise auxiliary kernel library
	AuxKlibInitialize();

	ULONG BufferSize = 0x00;
	NTSTATUS Status = AuxKlibQueryModuleInformation(&BufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
	if (NT_ERROR(Status) || BufferSize == 0x00)
		return STATUS_UNSUCCESSFUL;

	PAUX_MODULE_EXTENDED_INFO ExtendedInfo = ExAllocatePool2(POOL_FLAG_PAGED, BufferSize, WKI_MM_TAG);
	if (ExtendedInfo == NULL)
		return STATUS_NO_MEMORY;
	Status = AuxKlibQueryModuleInformation(&BufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), (PVOID)ExtendedInfo);

	// Parse all modules
	for (UINT32 cx = 0x00; cx < (BufferSize / sizeof(AUX_MODULE_EXTENDED_INFO)); cx++) {
		if (strcmp("\\SystemRoot\\system32\\ntoskrnl.exe", (CONST PCHAR)ExtendedInfo[cx].FullPathName) == 0x00) {
			WkiGlobal.KernelBase = (UINT64)ExtendedInfo[cx].BasicInfo.ImageBase;
			break;
		}
	}
	NT_ASSERT(WkiGlobal.KernelBase != 0x00);
	
	ExFreePoolWithTag((PVOID)ExtendedInfo, WKI_MM_TAG);
	return STATUS_SUCCESS;
}


_Use_decl_annotations_
EXTERN_C NTSTATUS WkiInitialise() {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check if already initialised
	if (WkiInitialised)
		return STATUS_SUCCESS;

	// Initialise single list entries
	InitializeListHead(&WkiGlobal.SymbolHead);

	// Early stack variable declaration for goto usage
	NTSTATUS Status      = STATUS_SUCCESS;
	HANDLE   RegistryKey = NULL;

	// Get the current OS version
	Status = WkipGetInitialRegistryKey(&RegistryKey);
	if (NT_ERROR(Status))
		goto exit;

	// Get all symbols
	Status = WkipGetSymbolEntries(RegistryKey);
	if (NT_ERROR(Status))
		goto exit;

	// Get kernel image base address
	Status = WkipGetSystemImageBase();
	if (NT_ERROR(Status))
		goto exit;

	// Finish
	WkiInitialised = TRUE;
exit:
	if (NT_ERROR(Status)) {
		if (RegistryKey != NULL)
			ZwClose(RegistryKey);
		WkiUninitialise();
	}
	return Status;
}


_Use_decl_annotations_
EXTERN_C VOID WkiUninitialise() {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check if everything has been initialised.
	if (!WkiInitialised)
		return;
	
	while (!IsListEmpty(&WkiGlobal.SymbolHead)) {
		PLIST_ENTRY       SymbolHead = RemoveHeadList(&WkiGlobal.SymbolHead);
		PWKI_SYMBOL_ENTRY Symbol     = CONTAINING_RECORD(SymbolHead, WKI_SYMBOL_ENTRY, List);
		ExFreePoolWithTag(Symbol, WKI_MM_TAG);
	}

	WkiInitialised = FALSE;
}


#pragma warning(disable: 4706)
_Use_decl_annotations_
EXTERN_C PVOID WkiGetSymbol(
	_In_ LPCSTR SymbolName
) {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	if (SymbolName == NULL)
		return NULL;

	// Check that there is at least one entry provided
	if (WkiGlobal.NumberOfSymbols == 0x00)
		return NULL;

	// Get the DJB2 hash of the requested symbol
	DWORD Hash = 0x1505;
	INT   c    = 0x0000;
	LPSTR d    = (LPSTR)SymbolName;
	while (c = *d++)
		Hash = ((Hash << 5) + Hash) + c;

	// Parse the list to find the entry
	PLIST_ENTRY Head = WkiGlobal.SymbolHead.Flink;
	do {
		PWKI_SYMBOL_ENTRY Entry = CONTAINING_RECORD(Head, WKI_SYMBOL_ENTRY, List);
		if (Entry == NULL)
			break;

		if (Entry->Body.DJB == Hash)
			return (PVOID)(WkiGlobal.KernelBase + (UINT64)Entry->Body.RVA);


		if (Head->Flink == &WkiGlobal.SymbolHead)
			break;
		Head = Head->Flink;
	} while (TRUE);
	return NULL;
}
#pragma warning(default: 4706)


_Use_decl_annotations_
EXTERN_C UINT64 WkiReadValue(
	_In_ PVOID  Address,
	_In_ UINT16 Size
) {

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check size is valid
	if (Size < sizeof(UCHAR) || Size > sizeof(UINT64))
		return 0x00;

	// Check address is valid
	if (!MmIsAddressValid(Address))
		return 0x00;

	// Read memory
	UINT64 Out = 0x00;
	RtlCopyMemory(&Out, Address, Size);

	return Out;
}
