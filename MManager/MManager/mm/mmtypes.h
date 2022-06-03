/*+================================================================================================
Module Name: mmtypes.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
Windows Kernel Memory Manager specific types.
Most of the types have been found using PDB and via WinDBG.
The RESym tool has been used as well: https://github.com/ergrelet/resym

Types are based on Windows 10 19044.1706 and they may differ from one version to another,
use with caution.
================================================================================================+*/

#ifndef __X_MM_MMTYPES_H_GUARD__
#define __X_MM_MMTYPES_H_GUARD__

#ifndef _NTIFS_
#include <ntifs.h>
#endif // !_NTIFS_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union

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

typedef struct _RTL_BITMAP_EX {
	ULONG64  SizeOfBitMap;
	PULONG64 Buffer;
} RTL_BITMAP_EX, * PRTL_BITMAP_EX;

typedef struct _RTL_AVL_TREE {
	PRTL_BALANCED_NODE Root;
} RTL_AVL_TREE, * PRTL_AVL_TREE;

typedef struct _MMPTE_HARDWARE {
	ULONG64 Valid : 1;
	ULONG64 Dirty1 : 1;
	ULONG64 Owner : 1;
	ULONG64 WriteThrough : 1;
	ULONG64 CacheDisable : 1;
	ULONG64 Accessed : 1;
	ULONG64 Dirty : 1;
	ULONG64 LargePage : 1;
	ULONG64 Global : 1;
	ULONG64 CopyOnWrite : 1;
	ULONG64 Unused : 1;
	ULONG64 Write : 1;
	ULONG64 PageFrameNumber : 36;
	ULONG64 ReservedForHardware : 4;
	ULONG64 ReservedForSoftware : 4;
	ULONG64 WsleAge : 4;
	ULONG64 WsleProtection : 3;
	ULONG64 NoExecute : 1;
} MMPTE_HARDWARE, * PMMPTE_HARDWARE;

typedef struct _MMPTE_PROTOTYPE {
	ULONG64 Valid : 1;
	ULONG64 DemandFillProto : 1;
	ULONG64 HiberVerifyConverted : 1;
	ULONG64 ReadOnly : 1;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 Combined : 1;
	ULONG64 Unused1 : 4;
	ULONG64 ProtoAddress : 48;
} MMPTE_PROTOTYPE, * PMMPTE_PROTOTYPE;

typedef struct _MMPTE_SOFTWARE {
	ULONG64 Valid : 1;
	ULONG64 PageFileReserved : 1;
	ULONG64 PageFileAllocated : 1;
	ULONG64 ColdPage : 1;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 Transition : 1;
	ULONG64 PageFileLow : 4;
	ULONG64 UsedPageTableEntries : 10;
	ULONG64 ShadowStack : 1;
	ULONG64 Unused : 5;
	ULONG64 PageFileHigh : 32;
} MMPTE_SOFTWARE, * PMMPTE_SOFTWARE;

typedef struct _MMPTE_TIMESTAMP {
	ULONG64 MustBeZero : 1;
	ULONG64 Unused : 3;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 Transition : 1;
	ULONG64 PageFileLow : 4;
	ULONG64 Reserved : 16;
	ULONG64 GlobalTimeStamp : 32;
} MMPTE_TIMESTAMP, * PMMPTE_TIMESTAMP;

typedef struct _MMPTE_TRANSITION {
	ULONG64 Valid : 1;
	ULONG64 Write : 1;
	ULONG64 Spare : 1;
	ULONG64 IoTracker : 1;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 Transition : 1;
	ULONG64 PageFrameNumber : 36;
	ULONG64 Unused : 16;
} MMPTE_TRANSITION, * PMMPTE_TRANSITION;

typedef struct _MMPTE_SUBSECTION {
	ULONG64 Valid : 1;
	ULONG64 Unused0 : 3;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 ColdPage : 1;
	ULONG64 Unused1 : 3;
	ULONG64 ExecutePrivilege : 1;
	ULONG64 SubsectionAddress : 48;
} MMPTE_SUBSECTION, * PMMPTE_SUBSECTION;

typedef struct _MMPTE_LIST {
	ULONG64 Valid : 1;
	ULONG64 OneEntry : 1;
	ULONG64 filler0 : 2;
	ULONG64 SwizzleBit : 1;
	ULONG64 Protection : 5;
	ULONG64 Prototype : 1;
	ULONG64 Transition : 1;
	ULONG64 filler1 : 16;
	ULONG64 NextEntry : 36;
} MMPTE_LIST, * PMMPTE_LIST;

typedef struct _MMPTE {
	union {
		ULONG64          Long;
		volatile ULONG64 VolatileLong;
		MMPTE_HARDWARE   Hard;
		MMPTE_PROTOTYPE  Proto;
		MMPTE_SOFTWARE   Soft;
		MMPTE_TIMESTAMP  TimeStamp;
		MMPTE_TRANSITION Trans;
		MMPTE_SUBSECTION Subsect;
		MMPTE_LIST       List;
	} u;
} MMPTE, * PMMPTE;

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

typedef struct _MMSECTION_FLAGS { /* Size=0x4 */
	union {
		ULONG BeingDeleted : 1;
		ULONG BeingCreated : 1;
		ULONG BeingPurged : 1;
		ULONG NoModifiedWriting : 1;
		ULONG FailAllIo : 1;
		ULONG Image : 1;
		ULONG Based : 1;
		ULONG File : 1;
		ULONG AttemptingDelete : 1;
		ULONG PrefetchCreated : 1;
		ULONG PhysicalMemory : 1;
		ULONG ImageControlAreaOnRemovableMedia : 1;
		ULONG Reserve : 1;
		ULONG Commit : 1;
		ULONG NoChange : 1;
		ULONG WasPurged : 1;
		ULONG UserReference : 1;
		ULONG GlobalMemory : 1;
		ULONG DeleteOnClose : 1;
		ULONG FilePointerNull : 1;
		ULONG PreferredNode : 6;
		ULONG GlobalOnlyPerSession : 1;
		ULONG UserWritable : 1;
		ULONG SystemVaAllocated : 1;
		ULONG PreferredFsCompressionBoundary : 1;
		ULONG UsingFileExtents : 1;
		ULONG PageSize64K : 1;
	};
} MMSECTION_FLAGS, * PMMSECTION_FLAGS;

typedef struct _MMSECTION_FLAGS2 { /* Size=0x4 */
	USHORT PartitionId : 10;
	union {
		UCHAR NoCrossPartitionAccess : 1;
		UCHAR SubsectionCrossPartitionReferenceOverflow : 1;
	};
} MMSECTION_FLAGS2, * PMMSECTION_FLAGS2;

typedef struct _MM_PRIVATE_VAD_FLAGS {
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
} MM_PRIVATE_VAD_FLAGS, * PMM_PRIVATE_VAD_FLAGS;

typedef struct _MM_GRAPHICS_VAD_FLAGS {
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
} MM_GRAPHICS_VAD_FLAGS, * PMM_GRAPHICS_VAD_FLAGS;

typedef struct _MM_SHARED_VAD_FLAGS {
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
} MM_SHARED_VAD_FLAGS, * PMM_SHARED_VAD_FLAGS;

typedef struct _MMVAD_FLAGS1 {
	ULONG CommitCharge : 31;
	ULONG MemCommit : 1;
} MMVAD_FLAGS1, * PMMVAD_FLAGS1;

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

typedef struct _MMSECURE_FLAGS {
	ULONG64 ReadOnly : 1;
	ULONG64 ReadWrite : 1;
	ULONG64 SecNoChange : 1;
	ULONG64 NoDelete : 1;
	ULONG64 RequiresPteReversal : 1;
	ULONG64 ExclusiveSecure : 1;
	ULONG64 UserModeOnly : 1;
	ULONG64 NoInherit : 1;
	ULONG64 CheckVad : 1;
	ULONG64 Spare : 3;
} MMSECURE_FLAGS, * PMMSECURE_FLAGS;

typedef struct _MMADDRESS_LIST {
	union {
		MMSECURE_FLAGS Flags;
		ULONG64        FlagsLong;
		ULONG64        StartVa;
	} u1;
	ULONG64 EndVa;
} MMADDRESS_LIST, * PMMADDRESS_LIST;

typedef struct _MMPFNENTRY1 {
	UCHAR PageLocation : 3;
	UCHAR WriteInProgress : 1;
	UCHAR Modified : 1;
	UCHAR ReadInProgress : 1;
	UCHAR CacheAttribute : 2;
} MMPFNENTRY1, * PMMPFNENTRY1;

typedef struct _MMPFNENTRY3 {
	UCHAR Priority : 3;
	UCHAR OnProtectedStandby : 1;
	UCHAR InPageError : 1;
	UCHAR SystemChargedPage : 1;
	UCHAR RemovalRequested : 1;
	UCHAR ParityError : 1;
} MMPFNENTRY3, * PMMPFNENTRY3;

typedef struct _MI_ACTIVE_PFN {
	union {
		struct {
			ULONG64 Tradable : 1;
			ULONG64 NonPagedBuddy : 43;
		} Leaf;
		struct {
			ULONG64 Tradable : 1;
			ULONG64 WsleAge : 3;
			ULONG64 OldestWsleLeafEntries : 10;
			ULONG64 OldestWsleLeafAge : 3;
			ULONG64 NonPagedBuddy : 43;
		} PageTable;
		ULONG64 EntireActiveField;
	} DUMMYUNIONNAME;
} MI_ACTIVE_PFN, * PMI_ACTIVE_PFN;

typedef struct _MI_STORE_INPAGE_COMPLETE_FLAGS {
	union {
		ULONG EntireFlags;
		ULONG StoreFault : 1;
		ULONG LowResourceFailure : 1;
		ULONG Spare : 14;
		ULONG RemainingPageCount : 16;
	};
} MI_STORE_INPAGE_COMPLETE_FLAGS, * PMI_STORE_INPAGE_COMPLETE_FLAGS;

typedef struct _MIPFNBLINK {
	union {
		ULONG64 Blink : 36;
		ULONG64 NodeBlinkHigh : 20;
		ULONG64 TbFlushStamp : 4;
		ULONG64 Unused : 2;
		ULONG64 PageBlinkDeleteBit : 1;
		ULONG64 PageBlinkLockBit : 1;
		ULONG64 ShareCount : 62;
		ULONG64 PageShareCountDeleteBit : 1;
		ULONG64 PageShareCountLockBit : 1;
		ULONG64 EntireField;

		INT64   LockNotUsed : 62;
		INT64   DeleteBit : 1;
		INT64   LockBit : 1;
		INT64   Lock;
	};
} MIPFNBLINK, * PMIPFNBLINK;

typedef struct _MMINPAGE_FLAGS {
	ULONG GetExtents : 1;
	ULONG PrefetchSystemVmType : 2;
	ULONG VaPrefetchReadBlock : 1;
	ULONG CollidedFlowThrough : 1;
	ULONG ForceCollisions : 1;
	ULONG InPageExpanded : 1;
	ULONG IssuedAtLowPriority : 1;
	ULONG FaultFromStore : 1;
	ULONG PagePriority : 3;
	ULONG ClusteredPagePriority : 3;
	ULONG MakeClusterValid : 1;
	ULONG PerformRelocations : 1;
	ULONG ZeroLastPage : 1;
	ULONG UserFault : 1;
	ULONG StandbyProtectionNeeded : 1;
	ULONG PteChanged : 1;
	ULONG PageFileFault : 1;
	ULONG PageFilePageHashActive : 1;
	ULONG CoalescedIo : 1;
	ULONG VmLockNotNeeded : 1;
	ULONG Spare0 : 1;
	ULONG Spare1 : 6;
} MMINPAGE_FLAGS, * PMMINPAGE_FLAGS;

typedef struct _MMPFN {
	union {
		LIST_ENTRY ListEntry;
		RTL_BALANCED_NODE TreeNode;
		struct {
			union {
				SINGLE_LIST_ENTRY NextSlistPfn;
				PVOID             Next;
				ULONG64           Flink : 36;
				ULONG64           NodeFlinkHigh : 28;
				MI_ACTIVE_PFN     Active;
			} u1;
			union {
				PMMPTE* PteAddress;
				ULONG64 PteLong;
			};
			MMPTE OriginalPte;
		};
	};
	MIPFNBLINK u2;
	union {
		struct {
			USHORT ReferenceCount;
			MMPFNENTRY1 e1;
			MMPFNENTRY3 e3;
		};
		struct {
			USHORT ReferenceCount;
		} e2;
		struct {
			ULONG EntireField;
		}e4;
	} u3;
	USHORT NodeBlinkLow;
	union {
		UCHAR Unused : 4;
		UCHAR Unused2 : 4;
	};
	union {
		UCHAR ViewCount;
		UCHAR NodeFlinkLow;
		UCHAR ModifiedListBucketIndex : 4;
		UCHAR AnchorLargePageSize : 2;
	};
	union {
		ULONG64 PteFrame : 36;
		ULONG64 ResidentPage : 1;
		ULONG64 Unused1 : 1;
		ULONG64 Unused2 : 1;
		ULONG64 Partition : 10;
		ULONG64 FileOnly : 1;
		ULONG64 PfnExists : 1;
		ULONG64 Spare : 9;
		ULONG64 PageIdentity : 3;
		ULONG64 PrototypePte : 1;
		ULONG64 EntireField;
	} u4;
} MMPFN, * PMMPFN;

typedef struct _MI_HARD_FAULT_STATE {
	PMMPFN                          SwapPfn;
	MI_STORE_INPAGE_COMPLETE_FLAGS  StoreFlags;
} MI_HARD_FAULT_STATE, * PMI_HARD_FAULT_STATE;

typedef struct _MODWRITER_FLAGS {
	ULONG KeepForever : 1;
	ULONG Networked : 1;
	ULONG IoPriority : 3;
	ULONG ModifiedStoreWrite : 1;
} MODWRITER_FLAGS, * PMODWRITER_FLAGS;

typedef struct _SEGMENT_FLAGS {
	union {
		USHORT TotalNumberOfPtes4132 : 10;
		USHORT Spare0 : 1;
		USHORT SessionDriverProtos : 1;
		USHORT LargePages : 1;
		USHORT DebugSymbolsLoaded : 1;
		USHORT WriteCombined : 1;
		USHORT NoCache : 1;
		USHORT Short0;
	};
	union {
		UCHAR Spare : 1;
		UCHAR DefaultProtectionMask : 5;
		UCHAR Binary32 : 1;
		UCHAR ContainsDebug : 1;
		UCHAR UChar1;
	};
	union {
		UCHAR ForceCollision : 1;
		UCHAR ImageSigningType : 3;
		UCHAR ImageSigningLevel : 4;
		UCHAR UChar2;
	};
} SEGMENT_FLAGS, * PSEGMENT_FLAGS;

typedef struct _MMEXTEND_INFO {
	ULONG64 CommittedSize;
	ULONG   ReferenceCount;
} MMEXTEND_INFO, * PMMEXTEND_INFO;

typedef struct _EX_FAST_REF {
	union {
		PVOID Object;
		ULONG64 RefCnt : 4;
		ULONG64 Value;
	};
} EX_FAST_REF, * PEX_FAST_REF;

typedef struct _SECTION_IMAGE_INFORMATION { /* Size=0x40 */
	PVOID   TransferAddress;
	ULONG   ZeroBits;
	ULONG64 MaximumStackSize;
	ULONG64 CommittedStackSize;
	ULONG   SubSystemType;
	union {
		struct {
			USHORT SubSystemMinorVersion;
			USHORT SubSystemMajorVersion;
		};
		ULONG SubSystemVersion;
	};
	union {
		struct {
			USHORT MajorOperatingSystemVersion;
			USHORT MinorOperatingSystemVersion;
		};
		ULONG OperatingSystemVersion;
	};
	USHORT ImageCharacteristics;
	USHORT DllCharacteristics;
	USHORT Machine;
	UCHAR  ImageContainsCode;
	union {
		UCHAR ImageFlags;
		UCHAR ComPlusNativeReady : 1;
		UCHAR ComPlusILOnly : 1;
		UCHAR ImageDynamicallyRelocated : 1;
		UCHAR ImageMappedFlat : 1;
		UCHAR BaseBelow4gb : 1;
		UCHAR ComPlusPrefer32bit : 1;
		UCHAR Reserved : 2;
	};
	ULONG LoaderFlags;
	ULONG ImageFileSize;
	ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, * PSECTION_IMAGE_INFORMATION;

typedef struct _MI_EXTRA_IMAGE_INFORMATION { /* Size=0x10 */
	ULONG SizeOfHeaders;
	ULONG SizeOfImage;
	ULONG TimeDateStamp;
	union {
		ULONG ImageCetShadowStacksReady : 1;
		ULONG ImageCetShadowStacksStrictMode : 1;
		ULONG ImageCetSetContextIpValidationRelaxedMode : 1;
		ULONG ImageCetDynamicApisAllowInProc : 1;
		ULONG ImageCetDowngradeReserved1 : 1;
		ULONG ImageCetDowngradeReserved2 : 1;
		ULONG Spare : 26;
	};
} MI_EXTRA_IMAGE_INFORMATION, * PMI_EXTRA_IMAGE_INFORMATION;

typedef struct _MI_SECTION_IMAGE_INFORMATION {
	SECTION_IMAGE_INFORMATION  ExportedImageInformation;
	MI_EXTRA_IMAGE_INFORMATION InternalImageInformation;
} MI_SECTION_IMAGE_INFORMATION, * PMI_SECTION_IMAGE_INFORMATION;

typedef struct _SEGMENT {
	struct CONTROL_AREA* ControlArea;
	ULONG         TotalNumberOfPtes;
	SEGMENT_FLAGS SegmentFlags;
	ULONG64       NumberOfCommittedPages;
	ULONG64       SizeOfSegment;
	union {
		PMMEXTEND_INFO ExtendInfo;
		PVOID          BasedAddress;
	};
	EX_PUSH_LOCK SegmentLock;
	union {
		ULONG64 ImageCommitment;
		ULONG   CreatingProcessId;
	} u1;
	union {
		PMI_SECTION_IMAGE_INFORMATION ImageInformation;
		PVOID                         FirstMappedVa;
	} u2;
	PMMPTE PrototypePte;
} SEGMENT, * PSEGMENT;

typedef struct _CONTROL_AREA {
	PSEGMENT Segment;
	union {
		LIST_ENTRY ListHead;
		PVOID AweContext;
	};
	ULONG64 NumberOfSectionReferences;
	ULONG64 NumberOfPfnReferences;
	ULONG64 NumberOfMappedViews;
	ULONG64 NumberOfUserReferences;
	union {
		ULONG LongFlags;
		MMSECTION_FLAGS Flags;
	} u;
	union {
		ULONG LongFlags;
		MMSECTION_FLAGS2 Flags;
	} u1;
	EX_FAST_REF FilePointer;
	volatile LONG ControlAreaLock;
	ULONG ModifiedWriteCount;
	struct MI_CONTROL_AREA_WAIT_BLOCK* WaitList;
	union {
		struct {
			union {
				ULONG NumberOfSystemCacheViews;
				ULONG ImageRelocationStartBit;
			};
			union {
				volatile LONG WritableUserReferences;
				ULONG ImageRelocationSizeIn64k : 16;
				ULONG SystemImage : 1;
				ULONG CantMove : 1;
				ULONG StrongCode : 2;
				ULONG BitMap : 2;
				ULONG ImageActive : 1;
				ULONG ImageBaseOkToReuse : 1;
			};
			union {
				ULONG FlushInProgressCount;
				ULONG NumberOfSubsections;
				struct MI_IMAGE_SECURITY_REFERENCE* SeImageStub;
			};
		};
	} u2;
	EX_PUSH_LOCK FileObjectLock;
	volatile ULONG64 LockedPages;
	union {
		ULONG64 IoAttributionContext : 61;
		ULONG64 Spare : 3;
		ULONG64 ImageCrossPartitionCharge;
		ULONG64 CommittedPageCount : 36;
	} u3;
} CONTROL_AREA, * PCONTROL_AREA;

typedef struct _MI_CONTROL_AREA_WAIT_BLOCK {
	struct MI_CONTROL_AREA_WAIT_BLOCK* Next;
	ULONG                               WaitReason;
	ULONG                               WaitResponse;
	KGATE                               Gate;
} MI_CONTROL_AREA_WAIT_BLOCK, * PMI_CONTROL_AREA_WAIT_BLOCK;

typedef struct _IMAGE_SECURITY_CONTEXT {
	union {
		PVOID PageHashes;
		ULONG64 Value;
		ULONG64 SecurityBeingCreated : 2;
		ULONG64 SecurityMandatory : 1;
		ULONG64 PageHashPointer : 61;
	};
} IMAGE_SECURITY_CONTEXT, * PIMAGE_SECURITY_CONTEXT;

typedef struct _MI_PROTOTYPE_PTES_NODE {
	RTL_BALANCED_NODE Node;
	union {
		struct {
			union {
				ULONG64 AllocationType : 3;
				ULONG64 Inserted : 1;
			};
		} e1;
		struct {
			ULONG64 PrototypePtesFlags;
		} e2;
	} u1;
} MI_PROTOTYPE_PTES_NODE, * PMI_PROTOTYPE_PTES_NODE;

typedef struct _MI_IMAGE_SECURITY_REFERENCE {
	MI_PROTOTYPE_PTES_NODE ProtosNode;
	PVOID DynamicRelocations;
	IMAGE_SECURITY_CONTEXT SecurityContext;
	union {
		PVOID   ImageFileExtents;
		ULONG64 ImageFileExtentsUlongPtr;
		ULONG64 FilesystemWantsRva : 1;
		ULONG64 Spare : 3;
	} u1;
	ULONG64 StrongImageReference;
} MI_IMAGE_SECURITY_REFERENCE, * PMI_IMAGE_SECURITY_REFERENCE;

typedef struct _MMMOD_WRITER_MDL_ENTRY {
	LIST_ENTRY      Links;
	union {
		IO_STATUS_BLOCK IoStatus;
	}  u;
	PIRP            Irp;
	MODWRITER_FLAGS u1;
	ULONG           StoreWriteRefCount;
	KAPC            StoreWriteCompletionApc;
	ULONG           ByteCount;
	ULONG           ChargedPages;
	PVOID           PagingFile; // PMMPAGING_FILE
	PFILE_OBJECT    File;
	PCONTROL_AREA   ControlArea;
	PERESOURCE      FileResource;
	LARGE_INTEGER   WriteOffset;
	LARGE_INTEGER   IssueTime;
	PVOID           Partition; // PMI_PARTITION
	PMDL            PointerMdl;
	MDL             Mdl;
	ULONG64         Page[1];
} MMMOD_WRITER_MDL_ENTRY, * PMMMOD_WRITER_MDL_ENTRY;

typedef struct _EPARTITION {
	PVOID MmPartition;
	PVOID CcPartition;
	PVOID ExPartition;
	LONG64 HardReferenceCount;
	LONG64 OpenHandleCount;
	LIST_ENTRY ActivePartitionLinks;
	struct EPARTITION* ParentPartition;
	WORK_QUEUE_ITEM TeardownWorkItem;
	EX_PUSH_LOCK TeardownLock;
	PEPROCESS SystemProcess;
	PVOID SystemProcessHandle;
	union {
		ULONG PartitionFlags;
		ULONG PairedWithJob : 1;
	};
} EPARTITION, * PEPARTITION;

typedef struct _MI_LARGEPAGE_VAD_INFO {
	UCHAR       LargeImageBias;
	UCHAR       Spare[3];
	ULONG64     ActualImageViewSize;
	PEPARTITION ReferencedPartition;
} MI_LARGEPAGE_VAD_INFO, * PMI_LARGEPAGE_VAD_INFO;

typedef struct _MI_PHYSICAL_VIEW {
	RTL_BALANCED_NODE PhysicalNode;
	struct MMVAD_SHORT* Vad;
	PVOID AweInfo;
	union {
		ULONG ViewPageSize : 2;
		PCONTROL_AREA ControlArea;
	} u1;
} MI_PHYSICAL_VIEW, * PMI_PHYSICAL_VIEW;

typedef struct _MI_SUB64K_FREE_RANGES {
	RTL_BITMAP_EX BitMap;
	LIST_ENTRY ListEntry;
	struct MMVAD_SHORT* Vad;
	ULONG SetBits;
	ULONG FullSetBits;
	union {
		ULONG SubListIndex : 2;
		ULONG Hint : 30;
	};
} MI_SUB64K_FREE_RANGES, * PMI_SUB64K_FREE_RANGES;

typedef struct _MI_VAD_EVENT_BLOCK {
	struct MI_VAD_EVENT_BLOCK* Next;

	union {
		KGATE Gate;
		MMADDRESS_LIST SecureInfo;
		RTL_BITMAP_EX BitMap;
		PVOID InPageSupport; // PMMINPAGE_SUPPORT
		MI_LARGEPAGE_VAD_INFO LargePage;
		MI_PHYSICAL_VIEW AweView;
		PETHREAD CreatingThread;
		MI_SUB64K_FREE_RANGES PebTeb;
		struct MMVAD_SHORT* PlaceholderVad;
	} DUMMYUNIONNAME;

	ULONG WaitReason;
} MI_VAD_EVENT_BLOCK, * PMI_VAD_EVENT_BLOCK;

typedef struct _MMVAD_SHORT {
	union {
		struct {
			struct MMVAD_SHORT* NextVad;
			PVOID ExtraCreateInfo;
		};
		RTL_BALANCED_NODE VadNode;
	};
	ULONG StartingVpn;
	ULONG EndingVpn;
	UCHAR StartingVpnHigh;
	UCHAR EndingVpnHigh;
	UCHAR CommitChargeHigh;
	UCHAR SpareNT64VadUChar;
	LONG  ReferenceCount;
	EX_PUSH_LOCK PushLock;
	union {
		ULONG LongFlags;
		MMVAD_FLAGS VadFlags;
		MM_PRIVATE_VAD_FLAGS PrivateVadFlags;
		MM_GRAPHICS_VAD_FLAGS GraphicsVadFlags;
		MM_SHARED_VAD_FLAGS SharedVadFlags;
		volatile ULONG VolatileVadLong;
	} u;
	union {
		ULONG LongFlags1;
		MMVAD_FLAGS1 VadFlags1;
	} u1;
	PMI_VAD_EVENT_BLOCK EventList;
} MMVAD_SHORT, * PMMVAD_SHORT;

typedef struct _MMVAD_FLAGS2 {
	union {
		ULONG FileOffset : 24;
		ULONG Large : 1;
		ULONG TrimBehind : 1;
		ULONG Inherit : 1;
		ULONG NoValidationNeeded : 1;
		ULONG PrivateDemandZero : 1;
		ULONG Spare : 3;
	};
} MMVAD_FLAGS2, * PMMVAD_FLAGS2;

typedef struct _MI_VAD_SEQUENTIAL_INFO { /* Size=0x8 */
	union {
		ULONG64 Length : 12;
		ULONG64 Vpn : 52;
	};
} MI_VAD_SEQUENTIAL_INFO, * PMI_VAD_SEQUENTIAL_INFO;

typedef struct _MMSUBSECTION_FLAGS {
	union {
		USHORT SubsectionAccessed : 1;
		USHORT Protection : 5;
		USHORT StartingSector4132 : 10;
	};
	union {
		USHORT SubsectionStatic : 1;
		USHORT GlobalMemory : 1;
		USHORT Spare : 1;
		USHORT OnDereferenceList : 1;
		USHORT SectorEndOffset : 12;
	};
} MMSUBSECTION_FLAGS, * PMMSUBSECTION_FLAGS;

typedef struct _MI_SUBSECTION_ENTRY1 {
	union {
		ULONG CrossPartitionReferences : 30;
		ULONG SubsectionMappedLarge : 2;
	};
} MI_SUBSECTION_ENTRY1, * PMI_SUBSECTION_ENTRY1;

typedef struct _MI_PER_SESSION_PROTOS {
	union {
		RTL_BALANCED_NODE SessionProtoNode;
		SINGLE_LIST_ENTRY FreeList;
		PVOID DriverAddress;
	};
	MI_PROTOTYPE_PTES_NODE ProtosNode;
	ULONG64 NumberOfPtes;
	union {
		ULONG SessionId;
		struct SUBSECTION* Subsection;
	};
	PMMPTE SubsectionBase;
	union {
		ULONG ReferenceCount;
		ULONG NumberOfPtesToFree;
	} u2;
} MI_PER_SESSION_PROTOS, * PMI_PER_SESSION_PROTOS;

typedef struct _SUBSECTION {
	PCONTROL_AREA ControlArea;
	struct MMPTE* SubsectionBase;
	struct SUBSECTION* NextSubsection;
	union {
		RTL_AVL_TREE                GlobalPerSessionHead;
		PMI_CONTROL_AREA_WAIT_BLOCK CreationWaitList;
		PMI_PER_SESSION_PROTOS      SessionDriverProtos;
	};
	union {
		ULONG              LongFlags;
		MMSUBSECTION_FLAGS SubsectionFlags;
	} u;
	ULONG StartingSector;
	ULONG NumberOfFullSectors;
	ULONG PtesInSubsection;
	union {
		MI_SUBSECTION_ENTRY1 e1;
		ULONG                EntireField;
	} u1;
	union {
		ULONG UnusedPtes : 30;
		ULONG ExtentQueryNeeded : 1;
		ULONG DirtyPages : 1;
	};
} SUBSECTION, * PSUBSECTION;

typedef struct _MMVAD {
	MMVAD_SHORT Core;
	union {
		ULONG LongFlags2;
		volatile MMVAD_FLAGS2 VadFlags2;
	} u2;
	PSUBSECTION Subsection;
	PMMPTE      FirstPrototypePte;
	PMMPTE      LastContiguousPte;
	LIST_ENTRY  ViewLinks;
	PEPROCESS   VadsProcess;
	union {
		MI_VAD_SEQUENTIAL_INFO SequentialVa;
		PMMEXTEND_INFO         ExtendedInfo;
	} u4;
	PFILE_OBJECT FileObject;
} MMVAD, * PMMVAD;

#endif // !__X_MM_MMTYPES_H_GUARD__
