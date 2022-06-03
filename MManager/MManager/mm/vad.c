/*+================================================================================================
Module Name: vad.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
All the routines used to deal with Virtual Address Descriptors (VADs).
APC must be disabled.

================================================================================================+*/

#include "vad.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, XMiInitializeVadTable)
#pragma alloc_text(PAGE, XMiUninitializeVadTable)
#pragma alloc_text(PAGE, XMiBuildVadTable)
#pragma alloc_text(PAGE, XMiGetVadNodeAbstractInfo)
#endif // ALLOC_PRAGMA

_Use_decl_annotations_
EXTERN_C NTSTATUS XMiInitializeVadTable(
	_In_  CONST PEPROCESS Process,
	_Out_ PXVAD_TABLE     XVadTable
) {
	if (Process == NULL)
		return STATUS_INVALID_PARAMETER_1;
	if (XVadTable == NULL)
		return STATUS_INVALID_PARAMETER_2;

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Allocate required data
	XVadTable->Process = (PEPROCESS)Process;
	InitializeListHead(&XVadTable->InsertOrderList);

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXTERN_C NTSTATUS XMiUninitializeVadTable(
	_In_ PXVAD_TABLE XVadTable
) {
	if (XVadTable == NULL)
		return STATUS_INVALID_PARAMETER_1;

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Check if there is any nodes
	if (XVadTable->NumberOfNodes == 0x00)
		return STATUS_SUCCESS;

	// Release all entries
	while (!IsListEmpty(&XVadTable->InsertOrderList)) {
		PLIST_ENTRY       Entry = RemoveTailList(&XVadTable->InsertOrderList);
		PXVAD_TABLE_ENTRY Vad = CONTAINING_RECORD(Entry, XVAD_TABLE_ENTRY, List);
		if (Vad != NULL) {
			ExFreePoolWithTag(Vad, XVAD_MM_TAG);
		}
	}

	RtlZeroMemory(XVadTable, sizeof(XVAD_TABLE));
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXTERN_C PXVAD_TABLE_ENTRY XMiBuildVadTable(
	_In_     PXVAD_TABLE       XVadTree,
	_In_opt_ PMMVAD            VadNode,
	_In_opt_ PXVAD_TABLE_ENTRY Parent,
	_In_     ULONG             Level
) {
	// Increase max level
	if (Level > XVadTree->MaximumLevel)
		XVadTree->MaximumLevel = Level;

	// Allocate memory to store the new node
	PXVAD_TABLE_ENTRY NewVadEntry = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(XVAD_TABLE_ENTRY), XVAD_MM_TAG);
	if (NewVadEntry == NULL)
		return NULL;
	NewVadEntry->Level = Level;
	XVadTree->NumberOfNodes++;

	// Get VadRoot if VadNode is NULL
	if (VadNode == NULL)
		VadNode = XMM_GET_PROCESS_VAD_ROOT(XVadTree->Process);

	// Get the information and insert into the list
	XMiGetVadNodeAbstractInfo(VadNode, NewVadEntry);
	InsertTailList(&XVadTree->InsertOrderList, &NewVadEntry->List);

	// Handle the root node
	if (Parent == NULL) {
		NewVadEntry->Parent = (struct XVAD_TABLE_ENTRY*)NewVadEntry;
		XVadTree->Root = NewVadEntry;
	}
	else
		NewVadEntry->Parent = (struct XVAD_TABLE_ENTRY*)Parent;

	// Get left and right nodes
	PMMVAD Left = (PMMVAD)VadNode->Core.VadNode.Left;
	PMMVAD Right = (PMMVAD)VadNode->Core.VadNode.Right;

	// Parse left and right nodes
	if (Left != NULL) {
		NewVadEntry->Left = (struct XVAD_TABLE_ENTRY*)XMiBuildVadTable(
			XVadTree,
			Left,
			NewVadEntry,
			Level + 1
		);
	}
	if (Right != NULL) {
		NewVadEntry->Right = (struct XVAD_TABLE_ENTRY*)XMiBuildVadTable(
			XVadTree,
			Right,
			NewVadEntry,
			Level + 1
		);
	}
	return NewVadEntry;
}

_Use_decl_annotations_
EXTERN_C VOID XMiGetVadNodeAbstractInfo(
	_In_ PMMVAD            VadNode,
	_In_ PXVAD_TABLE_ENTRY TableEntry
) {
	if (VadNode == NULL)
		return;
	if (TableEntry == NULL)
		return;

	// Ensure current IRQL allow paging.
	PAGED_CODE();

	// Easy store of the VAD Node
	TableEntry->VadNode = VadNode;

	// Calculate Virtual Page Number (VPN)
	TableEntry->StartingVpn = VadNode->Core.StartingVpn;
	TableEntry->EndingVpn = VadNode->Core.EndingVpn;
	if (VadNode->Core.StartingVpnHigh)
		TableEntry->StartingVpn |= ((ULONG64)VadNode->Core.StartingVpnHigh) << 32;
	if (VadNode->Core.EndingVpnHigh)
		TableEntry->EndingVpn |= ((ULONG64)VadNode->Core.EndingVpnHigh) << 32;

	// Get all flags
	TableEntry->VadFlags = VadNode->Core.u.VadFlags;
	TableEntry->VadFlags1 = VadNode->Core.u1.VadFlags1;
	TableEntry->VadFlags2 = VadNode->u2.VadFlags2;

	// Check for a control area. Applicable only in case this is mapped memory.
	if (!TableEntry->VadFlags.PrivateMemory) {
		if (VadNode->Subsection != NULL) {
			TableEntry->ControlArea = VadNode->Subsection->ControlArea;

			// Now check for the FileObject if any
			if (TableEntry->ControlArea->FilePointer.Value != 0x00) {
				TableEntry->FileObject = (PFILE_OBJECT)(TableEntry->ControlArea->FilePointer.Value & 0xFFFFFFFFFFFFFFF0);
				TableEntry->Name = &TableEntry->FileObject->FileName;
			}
		}
	}

	// Get commit charge
	TableEntry->CommitCharge = TableEntry->VadFlags1.CommitCharge;
	if (VadNode->Core.CommitChargeHigh)
		TableEntry->CommitCharge |= ((ULONG64)VadNode->Core.CommitChargeHigh) << 31;
}
