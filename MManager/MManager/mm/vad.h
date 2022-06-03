/*+================================================================================================
Module Name: vad.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
All the routines used to deal with Virtual Address Descriptors (VADs).
APC must be disabled.

================================================================================================+*/

#ifndef __X_VAD_H_GUARD__
#define __X_VAD_H_GUARD__

#include "mmtypes.h"

// XVAD Memory Pool Tag -- XVad
#define XVAD_MM_TAG (ULONG)0x64615658

// This is not a reliable way to get the VadRoot but could not find any better for the moment.
#define XMM_GET_PROCESS_VAD_ROOT(ps) (PMMVAD) *(PULONG64)((PUCHAR)ps + 0x7d8)

/// <summary>
/// Virtual Address Descriptor (VAD) abstraction structure.
/// </summary>
typedef struct _XVAD_TABLE_ENTRY {
	LIST_ENTRY               List;
	struct XVAD_TABLE_ENTRY* Parent;
	struct XVAD_TABLE_ENTRY* Left;
	struct XVAD_TABLE_ENTRY* Right;

	// Address of the VAD Node in kernel pool.
	union {
		PMMVAD VadNode;
		PVOID  Address;
	};

	PCONTROL_AREA ControlArea;  // Pointer to the CONTROL_AREA if mapped memory 
	PFILE_OBJECT  FileObject;   // Pointer to the FILE_OBJECT if mapped memory.
	ULONG64       StartingVpn;  // Start of Virtual Page Number(VPN).
	ULONG64       EndingVpn;    // Start of Virtual Page Number (VPN).
	ULONG64       CommitCharge;
	ULONG         Level;        // Depth level in the AFL tree.

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
		MMVAD_FLAGS1 VadFlags1;
		ULONG        LongVadFlags1;
	};

	// Third set of flags.
	union {
		MMVAD_FLAGS2 VadFlags2;
		ULONG        LongVadFlags2;
	};

	// Name of the mapped file, if any.
	PUNICODE_STRING Name;
} XVAD_TABLE_ENTRY, * PXVAD_TABLE_ENTRY;

/// <summary>
/// Global structure used to collect additional information about Virtual Address Descriptors (VADs).
/// </summary>
typedef struct _XVAD_TABLE {
	ULONG             MaximumLevel;       // Deepest level
	ULONG             NumberOfNodes;      // Total number of nodes
	ULONG64           TotalPrivateCommit; // Total private memory 
	ULONG64           TotalSharedCommit;  // Total shared memory 

	PEPROCESS         Process;            // Process linked to the VAD

	LIST_ENTRY        InsertOrderList;    // Order in which all nodes have been loaded
	PXVAD_TABLE_ENTRY Root;               // First entry of the table
} XVAD_TABLE, * PXVAD_TABLE;


_IRQL_requires_max_(APC_LEVEL)
EXTERN_C NTSTATUS XMiInitializeVadTable(
	_In_  CONST PEPROCESS Process,
	_Out_ PXVAD_TABLE     XVadTable
);


_IRQL_requires_max_(APC_LEVEL)
EXTERN_C NTSTATUS XMiUninitializeVadTable(
	_In_ PXVAD_TABLE XVadTree
);


_IRQL_requires_max_(APC_LEVEL)
EXTERN_C PXVAD_TABLE_ENTRY XMiBuildVadTable(
	_In_     PXVAD_TABLE       XVadTree,
	_In_opt_ PMMVAD            VadNode,
	_In_opt_ PXVAD_TABLE_ENTRY Parent,
	_In_     ULONG             Level
);

/// <summary>
/// Extract all the information from a Virtual Address Descriptor (VAD) node.
/// 
/// Most of the logic taken from C:\Program Files\WindowsApps\Microsoft.WinDbg_<whatever>\amd64\winxp\kdexts.dll
/// Interesting functions to RE from this module are:
///    - vad
///    - PrintVad
/// </summary>
/// <param name="VadNode">Pointer to an internal VAD structure.</param>
/// <param name="TableEntry">Pointer to global VAD table.</param>
EXTERN_C VOID XMiGetVadNodeAbstractInfo(
	_In_ PMMVAD            VadNode,
	_In_ PXVAD_TABLE_ENTRY TableEntry
);

#endif // !__X_VAD_H_GUARD__
