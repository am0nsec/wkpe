/*+================================================================================================
Module Name: main.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Device driver entry point.
================================================================================================+*/

#include "ki-globals.h"
#include "wki/wki.h"

/// <summary>
/// Device driver entry point.
/// </summary>
/// <param name="DriverObject">Pointer to the driver object.</param>
/// <param name="RegistryPath">Pointer to the registry path, if any.</param>
EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

/// <summary>
/// The Unload routine performs any operations that are necessary before the system unloads the driver.
/// </summary>
_IRQL_requires_max_(PASSIVE_LEVEL)
EXTERN_C VOID DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
);

/// <summary>
/// Test Windows Kernel Introspection initialisation and the successful retreival of required 
/// symbols for the list of memory pool tags.
/// </summary>
_IRQL_requires_max_(PASSIVE_LEVEL)
EXTERN_C VOID TestWKI(
	VOID
);

/// <summary>
/// List kernel memory ppol tags.
/// </summary>
_IRQL_requires_max_(PASSIVE_LEVEL)
EXTERN_C VOID ListPoolTags(
	VOID
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, TestWKI)
#pragma alloc_text(PAGE, ListPoolTags)
#endif // ALLOC_PRAGMA


_Use_decl_annotations_
EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KiDebug(("=================================================================\r\n"));
	KiDebug(("Module Name: Windows Kernel Introspection -- KM                  \r\n"));
	KiDebug(("Author     : Paul L. (@am0nsec)                                  \r\n"));
	KiDebug(("Origin     : https://github.com/am0nsec/wkpe/                    \r\n"));
	KiDebug(("Tested OS  : Windows 10 (20h2) - 19044.1889                      \r\n"));
	KiDebug(("             Windows 10 (20h2) - 19044.1766                      \r\n"));
	KiDebug(("             Windows 10 (20h2) - 19044.1706                      \r\n"));
	KiDebug(("=================================================================\r\n"));

	// Initialise stack variables
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING DeviceName   = RTL_CONSTANT_STRING(KI_DEVICE_NAME);
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KI_SYMBOLIC_LINK_NAME);

	PDEVICE_OBJECT DeviceObject = NULL;
	BOOLEAN        SymbolicLink = FALSE;

	// Set unload routine
	DriverObject->DriverUnload = DriverUnload;

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
			&KI_CLASS_GUID,
			&DeviceObject
		);
		if (!NT_SUCCESS(Status)) {
			KiDebug(("Failed to create device object (0x%08X).\r\n", Status));
			break;
		}

		Status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);
		if (!NT_SUCCESS(Status)) {
			KiDebug(("Failed to symbolic link (0x%08X).\r\n", Status));
			break;
		}
		SymbolicLink = TRUE;
	} while (FALSE);

	// Check for errors
	if (!NT_SUCCESS(Status)) {
		KiDebug(("Roll-back operations.\r\n"));
		if (SymbolicLink)
			IoDeleteSymbolicLink(&SymbolicName);
		if (DeviceObject != NULL)
			IoDeleteDevice(DeviceObject);
		KiDebug(("----------------------------------------------------------------\r\n"));
		return Status;
	}

	// Display final information and exit
	KiDebug(("Device name       : %wZ\r\n", DeviceName));
	KiDebug(("Symbolic link name: %wZ\r\n", SymbolicName));
	KiDebug(("-----------------------------------------------\r\n"));

	TestWKI();
	ListPoolTags();
	return Status;
}


_Use_decl_annotations_
EXTERN_C VOID DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
) {
	KiDebug(("-----------------------------------------------\r\n"));

	// Uninitialise WKI
	WkiUninitialise();

	// Delete the symbolic link and the device object
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KI_SYMBOLIC_LINK_NAME);
	IoDeleteSymbolicLink(&SymbolicName);
	IoDeleteDevice(DriverObject->DeviceObject);
}


_Use_decl_annotations_
EXTERN_C VOID TestWKI(
	VOID
) {
	KiDebug(("Start testing wki ...\r\n"));

	NTSTATUS Status = WkiInitialise();
	if (NT_ERROR(Status)) {
		KiDebug(("WkiInitialise failed \r\n"));
		WkiUninitialise();
		return;
	}

	// Make sure everything is available when required
	ASSERT(WkiGetSymbol("KeNumberProcessors"));
	ASSERT(WkiGetSymbol("ExPoolTagTables"));
	ASSERT(WkiGetSymbol("PoolTrackTableSize"));

	KiDebug(("Start testing wki ... ok\r\n"));
	KiDebug(("-----------------------------------------------\r\n"));
}

typedef struct _POOL_TRACKER_TABLE {
	INT32  Key;
	UINT64 NonPagedBytes;
	UINT64 NonPagedAllocs;
	UINT64 NonPagedFrees;
	UINT64 PagedBytes;
	UINT64 PagedAllocs;
	UINT64 PagedFrees;
} POOL_TRACKER_TABLE, *PPOOL_TRACKER_TABLE;


EXTERN_C VOID MakePrintableString(
	_In_  UINT32 Key,
	_Out_ PUCHAR String
) {

	for (UINT16 cx = 0x04; cx > 0x00; cx--) {

		INT Shift = (0x08 * (cx - 1));
		INT Char = (Key & (UINT32)((UINT32)0xFF << Shift)) >> Shift;

		if (isprint(Char))
			String[cx - 1] = (UCHAR)Char;
		else
			String[cx - 1] = (UCHAR)0x2E;
	}
}


_Use_decl_annotations_
/// <summary>
/// List Windows Kernel Memory Pool Tags.
/// </summary>
EXTERN_C VOID ListPoolTags(
	VOID
) {
	KiDebug(("List kernel memory pool tags ...\r\n"));

	// Get the number of processors on the system.
	PVOID XxKeNumberProcessors = WkiGetSymbol("KeNumberProcessors");
	if (XxKeNumberProcessors == NULL) {
		KiDebug(("Error \"KeNumberProcessors\" symbol not found.\r\n"));
		return;
	}

	// Get the list of Pool tables
	PVOID PoolTagTables = WkiGetSymbol("ExPoolTagTables");
	if (PoolTagTables == NULL) {
		KiDebug(("Error \"ExPoolTagTables\" symbol not found.\r\n"));
		return;
	}

	// Get the pointer to the size of the kernel pool table.
	PVOID XxPoolTrackTableSize = WkiGetSymbol("PoolTrackTableSize");
	if (XxPoolTrackTableSize == NULL) {
		KiDebug(("Error \"PoolTrackTableSize\" symbol not found.\r\n"));
		return;
	}

	// Get the pool block shift if any
	PVOID XxExpPoolBlockShift = WkiGetSymbol("ExpPoolBlockShift");
	
	// Get pool table expansion 
	PVOID XxPoolTrackTableExpansion     = WkiGetSymbol("PoolTrackTableExpansion");
	PVOID XxPoolTrackTableExpansionSize = WkiGetSymbol("PoolTrackTableExpansionSize");
	
	// Prepare all local stack variables required
	UINT64 PoolTableEntries = 0x01;
	if (XxKeNumberProcessors != NULL)
		PoolTableEntries = WkiReadValue(XxKeNumberProcessors, sizeof(UINT32));

	UINT64 PoolBlockShift   = 0x00;
	if (XxExpPoolBlockShift != NULL)
		PoolBlockShift = WkiReadValue(XxExpPoolBlockShift, sizeof(UINT64));

	UINT64 TrackTableExpansionEntries = 0x00;
	if (XxPoolTrackTableExpansionSize != NULL)
		TrackTableExpansionEntries = WkiReadValue(XxPoolTrackTableExpansionSize, sizeof(UINT64));

	PPOOL_TRACKER_TABLE TrackTableExpansion = NULL;
	if (XxPoolTrackTableExpansion != NULL)
		TrackTableExpansion = (PVOID)WkiReadValue(XxPoolTrackTableExpansion, sizeof(UINT64));

	UINT64 TrackTableEntries = WkiReadValue(XxPoolTrackTableSize, sizeof(UINT64));

	// Allocate memory to store all the data in order to filter the result.
	PPOOL_TRACKER_TABLE PoolTags = ExAllocatePool2(
		POOL_FLAG_NON_PAGED,
		((TrackTableEntries + TrackTableExpansionEntries) * sizeof(POOL_TRACKER_TABLE)),
		WKI_MM_TAG
	);
	if (PoolTags == NULL)
		return;

	// Parse all tracker entries
	UINT64 ValidEntries = 0x00;
	for (UINT64 cx = 0x00; cx < TrackTableEntries; cx++) {

		BOOLEAN Valid = TRUE;
		for (UINT64 dx = 0x00; dx < PoolTableEntries; dx++) {

			// Get proper tracker table
			PPOOL_TRACKER_TABLE TrackerTable = (PVOID)WkiReadValue(((PUCHAR)PoolTagTables + (0x8 * dx)), sizeof(PPOOL_TRACKER_TABLE));
			if (TrackerTable == NULL)
				continue;

			if (TrackerTable[cx].Key == 0x00)
				continue;

			PoolTags[cx].NonPagedAllocs += TrackerTable[cx].NonPagedAllocs;
			PoolTags[cx].NonPagedFrees  += TrackerTable[cx].NonPagedFrees;
			PoolTags[cx].NonPagedBytes  += TrackerTable[cx].NonPagedBytes;
			PoolTags[cx].PagedAllocs    += TrackerTable[cx].PagedAllocs;
			PoolTags[cx].PagedFrees     += TrackerTable[cx].PagedFrees;
			PoolTags[cx].PagedBytes     += TrackerTable[cx].PagedBytes;
			PoolTags[cx].Key             = TrackerTable[cx].Key;

			if (PoolTags[cx].Key != 0x00 && Valid) {
				ValidEntries++;
				Valid = FALSE;
			}
		}
	}
	if (TrackTableExpansion != NULL) {
		for (UINT64 cx = 0x00; cx < TrackTableExpansionEntries; cx++) {
			PoolTags[ValidEntries + cx].NonPagedAllocs += TrackTableExpansion[cx].NonPagedAllocs;
			PoolTags[ValidEntries + cx].NonPagedFrees  += TrackTableExpansion[cx].NonPagedFrees;
			PoolTags[ValidEntries + cx].NonPagedBytes  += TrackTableExpansion[cx].NonPagedBytes;
			PoolTags[ValidEntries + cx].PagedAllocs    += TrackTableExpansion[cx].PagedAllocs;
			PoolTags[ValidEntries + cx].PagedFrees     += TrackTableExpansion[cx].PagedFrees;
			PoolTags[ValidEntries + cx].PagedBytes     += TrackTableExpansion[cx].PagedBytes;
			PoolTags[ValidEntries + cx].Key             = TrackTableExpansion[cx].Key;
		}
	}

	// Display all information
	KdPrint(("                            NonPaged                                         Paged\r\n"));
	KdPrint((" Tag       Allocs       Frees      Diff         Used       Allocs       Frees      Diff         Used\r\n\r\n"));

	for (UINT64 cx = 0x00; cx < TrackTableEntries; cx++) {
		POOL_TRACKER_TABLE Entry = PoolTags[cx];
		if (Entry.Key == 0x00)
			continue;

		UCHAR KeyString[sizeof(UINT64)] = { 0x00 };
		MakePrintableString(Entry.Key, KeyString);

		KdPrint((" %s %11I64u %11I64u %9I64u %12I64d  %11I64u %11I64u %9I64u %12I64d\r\n",
			KeyString,
			Entry.NonPagedAllocs,
			Entry.NonPagedFrees,
			(Entry.NonPagedAllocs - Entry.NonPagedFrees),
			Entry.NonPagedBytes,
			Entry.PagedAllocs,
			Entry.PagedFrees,
			(Entry.PagedAllocs - Entry.PagedFrees),
			Entry.PagedBytes
		));
	}

	// Cleanup
	ExFreePoolWithTag(PoolTags, WKI_MM_TAG);
	KiDebug(("List kernel memory pool tags ... ok\r\n"));
	KiDebug(("-----------------------------------------------\r\n"));
}