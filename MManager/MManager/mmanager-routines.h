/*+================================================================================================
Module Name: mmanager-routines.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
Handle User-Mode IOCTL requests.

================================================================================================+*/

#ifndef __MMANAGER_H_GUARD__
#define __MMANAGER_H_GUARD__

#include "mmanager-globals.h"
#include "mm/vad.h"

#define XLATE_TO_UM_ADDRESS(UM, KM, Address) \
	(PVOID)(((PUCHAR)UM) + ((PUCHAR)Address - ((PUCHAR)KM)))

typedef struct _MMANAGER_VADLIST_ENTRY {
	struct {
		struct _MMANAGER_VADLIST_ENTRY* Flink;
		struct _MMANAGER_VADLIST_ENTRY* Blink;
	} List;

	ULONG64 Size;            // Size of the entry (structure + PWCHAR if filename)

	PVOID   VadAddress;      // Address of the VAD node
	ULONG   Level;           // Node depth level
	ULONG64 VpnStarting;     // Start of Virtual Page Number(VPN).
	ULONG64 VpnEnding;       // Start of Virtual Page Number (VPN).
	ULONG64 CommitCharge;    // Number of bytes commit
	ULONG64 CommitPageCount; // Number of pages commit

	// First set of flags.
	union {
		ULONG                 LongVadFlags;
		MMVAD_FLAGS           VadFlags;
		MM_PRIVATE_VAD_FLAGS  PrivateVadFlags;
		MM_GRAPHICS_VAD_FLAGS GraphicsVadFlags;
		MM_SHARED_VAD_FLAGS   SharedVadFlags;
	};
	// Second set of flags.
	union {
		ULONG        LongVadFlags1;
		MMVAD_FLAGS1 VadFlags1;
	};
	// Third set of flags.
	union {
		ULONG        LongVadFlags2;
		MMVAD_FLAGS2 VadFlags2;
	};

	ULONG   FileNameSize;  // Size of the filename
	WCHAR   FileName[ANYSIZE_ARRAY]; // Pointer to the filename
} MMANAGER_VADLIST_ENTRY, * PMMANAGER_VADLIST_ENTRY;


typedef struct _MMANAGER_VADLIST_HEADER {
	ULONG64           Size;               // Size of the data (header + all entries)
	ULONG             MaximumLevel;       // Deepest level
	ULONG             NumberOfNodes;      // Total number of nodes
	ULONG64           TotalPrivateCommit; // Total private memory 
	ULONG64           TotalSharedCommit;  // Total shared memory 
	PVOID             Eprocess;           //

	PMMANAGER_VADLIST_ENTRY First;
	PMMANAGER_VADLIST_ENTRY Last;
} MMANAGER_VADLIST_HEADER, * PMMANAGER_VADLIST_HEADER;


// Global variable to store the size of the UM structure.
extern ULONG64 VadTableSize;

// Global variable to build the UM structure.
extern XVAD_TABLE VadTable;


_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C NTSTATUS MmanIoctlFindProcessVads(
	_In_ PIRP               Irp,
	_In_ PIO_STACK_LOCATION Stack
);


_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C NTSTATUS MmanIoctlGetProcessVads(
	_In_  PIRP               Irp,
	_In_  PIO_STACK_LOCATION Stack,
	_Out_ ULONG_PTR*         BufferOutSize
);


_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C ULONG64 MmanpCalculateStructureSize(
	VOID
);

#endif // !__MMANAGER_H_GUARD__

