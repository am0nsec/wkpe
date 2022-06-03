/*+================================================================================================
Module Name: mmanager.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
User mode interface to query VAD list of a process.

================================================================================================+*/

#include <stdio.h>
#include "mmanager.h"


CMManager::CMManager() {
	// Open handle to the device driver
	this->m_DeviceHandle = ::CreateFileW(
		MMANAGER_DEVICE_NAME_UM,
		(GENERIC_READ | GENERIC_WRITE),
		0x00,
		NULL,
		OPEN_EXISTING,
		0x00,
		NULL
	);

}


CMManager::~CMManager() {
	if (this->m_DeviceHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(this->m_DeviceHandle);
	}
	if (this->m_ListHeader != NULL) {
		HeapFree(GetProcessHeap(), 0x00, this->m_ListHeader);
	}
}


_Use_decl_annotations_
BOOLEAN CMManager::IsDeviceReady() {
	return this->m_DeviceHandle != INVALID_HANDLE_VALUE;
}


_Use_decl_annotations_
BOOLEAN CMManager::FindProcessVads(
	_In_  CONST ULONG ProcessId
) {
	if (ProcessId == 0x00)
		return FALSE;
	if (!this->IsDeviceReady())
		return FALSE;

	// Make sure we do not allocate too much memory
	if (this->m_ListHeader != NULL)
		HeapFree(GetProcessHeap(), 0x00, this->m_ListHeader);
	
	// Send first IOCTL to get the amount of memory to allocate.
	ULONG64 BufferSize = 0x00;
	DWORD ReturnedBytes = 0x00;

	BOOL Success = ::DeviceIoControl(
		this->m_DeviceHandle,
		IOCTL_MMANAGER_FIND_PROCESS_VADS,
		(LPVOID)&ProcessId,
		sizeof(ULONG),
		&BufferSize,
		sizeof(ULONG64),
		&ReturnedBytes,
		NULL
	);
	if (!Success) {
		wprintf(L"IOCTL_MMANAGER_FIND_PROCESS_VADS failed (%d).\r\n", GetLastError());
		return Success;
	}

	// Allocate memory
	this->m_ListHeader = (MMANAGER_VADLIST_HEADER*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	if (this->m_ListHeader == NULL) {
		wprintf(L"Not enough memory (%d).\r\n", GetLastError());
		return FALSE;
	}

	// Call to get the data back
	ULONG64 UmAddress = (ULONG64)this->m_ListHeader;
	Success = ::DeviceIoControl(
		this->m_DeviceHandle,
		IOCTL_MMANAGER_GET_PROCESS_VADS,
		(PVOID)&UmAddress,
		sizeof(PVOID),
		this->m_ListHeader,
		(DWORD)BufferSize,
		&ReturnedBytes,
		NULL
	);
	if (!Success) {
		wprintf(L"IOCTL_MMANAGER_GET_PROCESS_VADS failed (%d).\r\n", GetLastError());
		HeapFree(GetProcessHeap(), 0x00, this->m_ListHeader);
	}
	return Success;
}


VOID CMManager::PrintProcessVads() {
	if (this->m_ListHeader == NULL)
		return;

	// Header of the table
	wprintf(L"VAD              Level  VPN Start    VPN End  Commit    Type         Protection         Pagefile/Image\r\n");
	wprintf(L"---              -----  ---------    -------  ------    ----         ----------         --------------\r\n");
	PMMANAGER_VADLIST_ENTRY Entry = this->m_ListHeader->First;
	do {
		// VAD node generic information
		wprintf(L"%p %5d  %9llx  %9llx  %-8I64d  %s",
			Entry->VadAddress,
			Entry->Level,

			Entry->VpnStarting,
			Entry->VpnEnding,
			Entry->CommitCharge,
			Entry->VadFlags.PrivateMemory != 0x00 ? L"Private " : L"Mapped  "
		);

		// VAD node type information
		switch ((MI_VAD_TYPE)Entry->VadFlags.VadType) {
		case VadDevicePhysicalMemory:
			wprintf(L"Phys ");
			break;
		case VadImageMap:
			wprintf(L"Exe  ");
			break;
		case VadAwe:
			wprintf(L"AWE  ");
			break;
		case VadWriteWatch:
			wprintf(L"WrtWatch  ");
			break;
		case VadLargePages:
			wprintf(L"LargePag  ");
			break;
		case VadRotatePhysical:
			wprintf(L"Rotate  ");
			break;
		case VadLargePageSection:
			wprintf(L"LargePagSec  ");
			break;
		case VadNone:
		default:
			wprintf(L"     ");
			break;
		}
		
		// VAD node permissions
		switch (Entry->VadFlags.Protection & MM_PROTECTION_OPERATION_MASK) {
		case MM_READONLY:
			wprintf(L"READONLY           ");
			break;
		case MM_EXECUTE:
			wprintf(L"EXECUTE            ");
			break;
		case MM_EXECUTE_READ:
			wprintf(L"EXECUTE_READ       ");
			break;
		case MM_READWRITE:
			wprintf(L"READWRITE          ");
			break;
		case MM_WRITECOPY:
			wprintf(L"WRITECOPY          ");
			break;
		case MM_EXECUTE_READWRITE:
			wprintf(L"EXECUTE_READWRITE  ");
			break;
		case MM_EXECUTE_WRITECOPY:
			wprintf(L"EXECUTE_WRITECOPY  ");
			break;
		}
		if ((Entry->VadFlags.Protection & ~MM_PROTECTION_OPERATION_MASK) != 0x00) {
			switch (Entry->VadFlags.Protection >> 0x03) {
			case (MM_NOCACHE >> 0x03):
				wprintf(L"NOCACHE            ");
				break;
			case (MM_GUARD_PAGE >> 0x03):
				wprintf(L"GUARD_PAGE         ");
				break;
			case (MM_NOACCESS >> 0x03):
				wprintf(L"NO_ACCESS          ");
				break;
			}
		}

		// Display file name if mapped
		if (Entry->FileName[0x00] != 0x00) {
			wprintf(L"%s", (PWCHAR)Entry->FileName);
		}
		else if (Entry->CommitPageCount != 0x00) {
			wprintf(L"Pagefile section, shared commit %#I64x", Entry->CommitPageCount);
		}
		wprintf(L"\r\n");
	} while (Entry = Entry->List.Flink);

	wprintf(L"\r\n");
	wprintf(L"EPROCESS     : 0x%p\r\n", this->m_ListHeader->Eprocess);
	wprintf(L"Total VADs   : %d\r\n", this->m_ListHeader->NumberOfNodes);
	wprintf(L"Maximum depth: %d\r\n", this->m_ListHeader->MaximumLevel);
	wprintf(L"\r\n");
}