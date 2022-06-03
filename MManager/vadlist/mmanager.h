/*+================================================================================================
Module Name: mmanager.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
User mode interface to query VAD list of a process.

================================================================================================+*/

#ifndef __MMANAGER_H_GUARD__
#define __MMANAGER_H_GUARD__

#include <Windows.h>
#include <winioctl.h>

// Query the VAD tree of a process
#define IOCTL_MMANAGER_FIND_PROCESS_VADS CTL_CODE( \
	0x8000,            /* DeviceType */\
	0x800,             /* Function   */\
	METHOD_OUT_DIRECT, /* Method     */\
	FILE_ANY_ACCESS    /* Access     */\
)

// Query the VAD tree of a process
#define IOCTL_MMANAGER_GET_PROCESS_VADS CTL_CODE( \
	0x8000,            /* DeviceType */\
	0x801,             /* Function   */\
	METHOD_OUT_DIRECT, /* Method     */\
	FILE_ANY_ACCESS    /* Access     */\
)

// User-mode name of the device driver
#define MMANAGER_DEVICE_NAME_UM L"\\\\.\\MManager"

#define MM_ZERO_ACCESS         0  // this value is not used.
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4  // bit 2 is set if this is writable.
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7

#define MM_NOCACHE            0x8
#define MM_GUARD_PAGE         0x10
#define MM_DECOMMIT           0x10   // NO_ACCESS, Guard page
#define MM_NOACCESS           0x18   // NO_ACCESS, Guard_page, nocache.
#define MM_UNKNOWN_PROTECTION 0x100  // bigger than 5 bits!

#define MM_INVALID_PROTECTION ((ULONG)-1)  // bigger than 5 bits!

#define MM_PROTECTION_WRITE_MASK     4
#define MM_PROTECTION_COPY_MASK      1
#define MM_PROTECTION_OPERATION_MASK 7 // mask off guard page and nocache.
#define MM_PROTECTION_EXECUTE_MASK   2

typedef enum _MI_VAD_TYPE {
	VadNone,
	VadDevicePhysicalMemory,
	VadImageMap,
	VadAwe,
	VadWriteWatch,
	VadLargePages,
	VadRotatePhysical,
	VadLargePageSection
} MI_VAD_TYPE, * PMI_VAD_TYPE;

typedef struct _MMVAD_FLAGS {
	ULONG Lock : 1;
	ULONG LockContended : 1;
	ULONG DeleteInProgress : 1;
	ULONG NoChange : 1;
	ULONG VadType : 3;
	ULONG Protection : 5;
	ULONG PreferredNode : 6;
	ULONG PageSize : 2;
	ULONG PrivateMemory : 1;
} MMVAD_FLAGS, * PMMVAD_FLAGS;

typedef struct _MMVAD_FLAGS1 {
	ULONG CommitCharge : 31;
	ULONG MemCommit : 1;
} MMVAD_FLAGS1, * PMMVAD_FLAGS1;

typedef struct _MMVAD_FLAGS2 {
	ULONG FileOffset : 24;
	ULONG Large : 1;
	ULONG TrimBehind : 1;
	ULONG Inherit : 1;
	ULONG NoValidationNeeded : 1;
	ULONG PrivateDemandZero : 1;
	ULONG Spare : 3;
} MMVAD_FLAGS2, * PMMVAD_FLAGS2;

typedef struct _MM_PRIVATE_VAD_FLAGS {
	union {
		ULONG Lock : 1;
		ULONG LockContended : 1;
		ULONG DeleteInProgress : 1;
		ULONG NoChange : 1;
		ULONG VadType : 3;
		ULONG Protection : 5;
		ULONG PreferredNode : 6;
		ULONG PageSize : 2;
		ULONG PrivateMemoryAlwaysSet : 1;
		ULONG WriteWatch : 1;
		ULONG FixedLargePageSize : 1;
		ULONG ZeroFillPagesOptional : 1;
		ULONG Graphics : 1;
		ULONG Enclave : 1;
		ULONG ShadowStack : 1;
		ULONG PhysicalMemoryPfnsReferenced : 1;
	};
} MM_PRIVATE_VAD_FLAGS, * PMM_PRIVATE_VAD_FLAGS;

typedef struct _MM_GRAPHICS_VAD_FLAGS {
	union {
		ULONG Lock : 1;
		ULONG LockContended : 1;
		ULONG DeleteInProgress : 1;
		ULONG NoChange : 1;
		ULONG VadType : 3;
		ULONG Protection : 5;
		ULONG PreferredNode : 6;
		ULONG PageSize : 2;
		ULONG PrivateMemoryAlwaysSet : 1;
		ULONG WriteWatch : 1;
		ULONG FixedLargePageSize : 1;
		ULONG ZeroFillPagesOptional : 1;
		ULONG GraphicsAlwaysSet : 1;
		ULONG GraphicsUseCoherentBus : 1;
		ULONG GraphicsNoCache : 1;
		ULONG GraphicsPageProtection : 3;
	};
} MM_GRAPHICS_VAD_FLAGS, * PMM_GRAPHICS_VAD_FLAGS;

typedef struct _MM_SHARED_VAD_FLAGS {
	union {
		ULONG Lock : 1;
		ULONG LockContended : 1;
		ULONG DeleteInProgress : 1;
		ULONG NoChange : 1;
		ULONG VadType : 3;
		ULONG Protection : 5;
		ULONG PreferredNode : 6;
		ULONG PageSize : 2;
		ULONG PrivateMemoryAlwaysClear : 1;
		ULONG PrivateFixup : 1;
		ULONG HotPatchAllowed : 1;
	};
} MM_SHARED_VAD_FLAGS, * PMM_SHARED_VAD_FLAGS;

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
	ULONG64 CommitPageCount; // Number of pages commited

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
} MMANAGER_VADLIST_ENTRY, *PMMANAGER_VADLIST_ENTRY;

typedef struct _MMANAGER_VADLIST_HEADER {
	ULONG64           Size;               // Size of the data (header + all entries)
	ULONG             MaximumLevel;       // Deepest level
	ULONG             NumberOfNodes;      // Total number of nodes
	ULONG64           TotalPrivateCommit; // Total private memory 
	ULONG64           TotalSharedCommit;  // Total shared memory 
	PVOID             Eprocess;           //

	PMMANAGER_VADLIST_ENTRY First;
	PMMANAGER_VADLIST_ENTRY Last;
} MMANAGER_VADLIST_HEADER, *PMMANAGER_VADLIST_HEADER;

class CMManager {
	
public:
	CMManager();
	~CMManager();

	_Must_inspect_result_
	BOOLEAN IsDeviceReady();

	_Must_inspect_result_
	BOOLEAN FindProcessVads(
		_In_ CONST ULONG ProcessId
	);

	VOID PrintProcessVads();

private:
	/// <summary>
	/// Handle to the device driver.
	/// </summary>
	HANDLE m_DeviceHandle{ INVALID_HANDLE_VALUE };

	/// <summary>
	/// Pointer to the header of the list.
	/// </summary>
	PMMANAGER_VADLIST_HEADER m_ListHeader{ NULL };
};

#endif // !__MMANAGER_H_GUARD__
