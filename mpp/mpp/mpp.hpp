/*+================================================================================================
Module Name: mpp.hpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Memory Patching Protection (MPP) Global Kernel Data.
================================================================================================+*/

#ifndef __MPP_MPP_H_GUARD__
#define __MPP_MPP_H_GUARD__

#include <ntifs.h>
#include <ntddk.h>
#include <wdmsec.h>

#ifndef _AUX_KLIB_H
#include <aux_klib.h>
#endif // !_AUX_KLIB_H


/// @brief MPP Global variables
namespace MppGlobals {
	/// @brief Kernel memory pool tag.
	extern ULONG PoolTag;

	/// @brief Name of the kernel device driver.
	extern UNICODE_STRING DeviceName;

	/// @brief Symbolic link name pointing to kernel device driver.
	extern UNICODE_STRING SymlinkName;

	/// @brief Unique Identifier for the kernel device driver - {F394F785-1D05-4C02-A3C5-6E3DC8F0BAD9}.
	extern CONST GUID DeviceId;
}


/// @brief List of IOCTLs for the dispatcher.
namespace MppIoctl {
	constexpr ULONG MppAddImageName() {
		return (ULONG)CTL_CODE(\
			0x8000,            /* DeviceType */\
			0x800,             /* Function   */\
			METHOD_IN_DIRECT,  /* Method     */\
			FILE_ANY_ACCESS    /* Access     */\
		);
	}

	constexpr ULONG MppRemoveImageName() {
		return (ULONG)CTL_CODE(\
			0x8000,            /* DeviceType */\
			0x801,             /* Function   */\
			METHOD_IN_DIRECT,  /* Method     */\
			FILE_ANY_ACCESS    /* Access     */\
		);
	}

	constexpr ULONG MppInitialiseKernelRoutines() {
		return (ULONG)CTL_CODE(\
			0x8000,            /* DeviceType */\
			0x802,             /* Function   */\
			METHOD_IN_DIRECT,  /* Method     */\
			FILE_ANY_ACCESS    /* Access     */\
		);
	}

	/// @brief Data structure for `MppInitialiseKernelRoutines` input buffer IOCTL.
	typedef struct _MPP_KERNEL_ROUTINES_OFFSETS {
		ULONG MiAddSecureEntry;
		ULONG MiObtainReferencedVadEx;
		ULONG MiUnlockAndDereferenceVad;
	} MPP_KERNEL_ROUTINES_OFFSETS, * PMPP_KERNEL_ROUTINES_OFFSETS;

	/// @brief Data structure for `MppRemoveImageName` input buffer IOCTL.
	typedef struct _MPP_REMOVE_IMAGE_NAME {
		SIZE_T Size;
		WCHAR  Name[260];
	} MPP_REMOVE_IMAGE_NAME, * PMPP_REMOVE_IMAGE_NAME;

	/// @brief Data structure for `MppAddImageName` input buffer IOCTL.
	typedef struct _MPP_ADD_IMAGE_NAME {
		SIZE_T Size;
		WCHAR  Name[260];
	} MPP_ADD_IMAGE_NAME, * PMPP_ADD_IMAGE_NAME;
}


/// @brief Kernel logger routines.
namespace MppLogger {

	/// @brief Default info logger.
	template<typename... Arguments>
	_inline void Info(Arguments... Args) {
		::KdPrint((Args...));
	}

	/// @brief Default error logger.
	template<typename... Arguments>
	_inline void Error(Arguments... Args) {
#if DBG
		::DbgPrint("[-::%ws::%d]\n\r", __FUNCTIONW__, __LINE__);
		::KdPrint((Args...));
#endif
	}
}


/// @brief Kernel memory alloc/free wrapper.
namespace MppMemory {

	/// @brief Wrapper to allocate memory within the kernel memory pool.
	template<typename T>
	_inline T MemAlloc(SIZE_T Size) {
		return (T)::ExAllocatePool2(
			POOL_FLAG_PAGED,
			Size,
			MppGlobals::PoolTag
		);
	}

	/// @brief Wrapper to free memory within the kernel memory pool.
	template<typename T>
	_inline VOID MemFree(T Data) {
		::ExFreePoolWithTag((PVOID)Data, MppGlobals::PoolTag);
	}

	/// @brief Check whether user input is large enough.
	/// @param IoStack IO Stack.
	/// @param Size    Minimum size to check.
	_inline BOOLEAN CheckInputBuffer(
		_In_ PIO_STACK_LOCATION IoStack,
		_In_ ULONG              Size
	) {
		return (IoStack->Parameters.DeviceIoControl.InputBufferLength <= Size);
	}
}


/// @brief Routines and data related to callbacks.
namespace MppCallbacks {

	/// @brief Callback for `PsRemoveLoadImageNotifyRoutine`.
	/// @param FullImageName Full name of the image being loaded.
	/// @param ProcessId     Process Id of the process loading the image.
	/// @param ImageInfo     Additional of the image being loaded. 
	VOID __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(PASSIVE_LEVEL)
	_IRQL_requires_max_(PASSIVE_LEVEL)
	LoadImageNotify(
		_In_opt_ PUNICODE_STRING FullImageName,
		_In_     HANDLE          ProcessId,
		_In_     PIMAGE_INFO     ImageInfo
	);

	/// @brief Weather the LoadImageNotify callbacks has been set.
	extern BOOLEAN LoadImage;
}


/// @brief Callback specific data.
namespace MppCallbackData {

	/// @brief Double-linked list of image names to protect .text section.
	extern LIST_ENTRY     HeadImageNames;

	/// @brief Number of image names to check for each modules being loaded.
	extern ULONG          NumberOfImageNames;

	/// @brief Spin lock for double-linked list of image names.
	extern KSPIN_LOCK     ImageNamesLock;

	/// @brief Add new image name in the `HeadImageNames` double-linked list..
	/// @param ImageName     Name of the image to add.
	/// @param ImageNameSize Size of the image name to add.	
	NTSTATUS __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(APC_LEVEL)
	_IRQL_requires_max_(APC_LEVEL)
	AddImageName(
		_In_  LPWSTR  ImageName,
		_In_  SIZE_T  ImageNameSize
	);

	/// @brief Remove image name from `HeadImageNames` double-linked list.
	/// @param ImageName Name of the entry to remove from the list.
	NTSTATUS __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(APC_LEVEL)
	_IRQL_requires_max_(APC_LEVEL)
	RemoveImageName(
		_In_ LPWSTR ImageName
	);

	/// @brief Structure used to store each image names to protect.
	typedef struct ImageNameEntry {
		LIST_ENTRY List;

		SIZE_T Size;
		WCHAR  Name[260];
	} ImageNameEntry;
}


/// @brief List of non-exported kernel routines required to secure .text section of a module.
namespace MppKernelRoutines {
	extern PVOID(STDMETHODCALLTYPE* MiObtainReferencedVadEx)(
		_In_  PVOID     Address,
		_In_  UCHAR     Flags,
		_Out_ NTSTATUS* Status
	);

	extern VOID(STDMETHODCALLTYPE* MiUnlockAndDereferenceVad)(
		_In_  PVOID VadObject
	);

	extern PVOID(STDMETHODCALLTYPE* MiAddSecureEntry) (
		_In_ PVOID VadObject,
		_In_ PVOID AddressStart,
		_In_ PVOID AddressEnd,
		_In_ ULONG ProbMode,
		_In_ ULONG Flags
	);

	/// @brief Base address of NT kernel image.
	extern UINT64 KernelBase;

	/// @brief Whether the kernel routines have been initialised.
	extern BOOLEAN RoutinesInitialised;

	/// @brief Get base address of the NT kernel image.
	/// @return 
	HRESULT __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(APC_LEVEL)
	_IRQL_requires_max_(APC_LEVEL)
	GetNtKernelBase();

	/// @brief Wrapper to update kernel routines function pointers.
	/// @param Function Function pointer to update.
	/// @param Offset   Offset of the function from NT kernel base address.
	template<typename T>
	_inline VOID UpdateFunctionPointer(
		_In_ T&    Function,
		_In_ ULONG Offset
	) {
		Function = reinterpret_cast<T>(
			MppKernelRoutines::KernelBase
			+ Offset
		);
	}
}


#endif // !__MPP_MPP_H_GUARD__
