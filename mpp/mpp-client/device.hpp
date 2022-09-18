/*+================================================================================================
Module Name: device.hpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Wrapper for MMP kernel device access.
================================================================================================+*/

#ifndef __MPPCLIENT_DEVICE_H_GUARD__
#define __MPPCLIENT_DEVICE_H_GUARD__

#include <Windows.h>

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

constexpr WCHAR AmsiImageName[] = L"amsi.dll\0";

/// @brief Data structure for `MppInitialiseKernelRoutines` input buffer IOCTL.
typedef struct _MPP_KERNEL_ROUTINES_OFFSETS {
	DWORD MiAddSecureEntry;
	DWORD MiObtainReferencedVadEx;
	DWORD MiUnlockAndDereferenceVad;
} MPP_KERNEL_ROUTINES_OFFSETS, *PMPP_KERNEL_ROUTINES_OFFSETS;

/// @brief Data structure for `MppRemoveImageName` input buffer IOCTL.
typedef struct _MPP_REMOVE_IMAGE_NAME {
	SIZE_T Size;
	WCHAR  Name[MAX_PATH];
} MPP_REMOVE_IMAGE_NAME, *PMPP_REMOVE_IMAGE_NAME;

/// @brief Data structure for `MppAddImageName` input buffer IOCTL.
typedef struct _MPP_ADD_IMAGE_NAME {
	SIZE_T Size;
	WCHAR  Name[MAX_PATH];
} MPP_ADD_IMAGE_NAME, *PMPP_ADD_IMAGE_NAME;

/// @brief Class wrapper to manage kernel device driver.
class CDeviceManager {
public:
	CDeviceManager() {
		// Open handle to the device driver
		this->hDevice = ::CreateFileA(
			"\\\\.\\MppDrv",
			(GENERIC_READ | GENERIC_WRITE),
			0x00,
			NULL,
			OPEN_EXISTING,
			0x00,
			NULL
		);
	}

	/// @brief Check if handle to kernel device driver is valid.
	_inline BOOLEAN
	_Must_inspect_result_
	IsDeviceReady() {
		return this->hDevice != INVALID_HANDLE_VALUE;
	}

	/// @brief IOCTL to send offset of kernel routines.
	/// @param KernelRoutines List of offsets of kernel routines.
	BOOLEAN
	_Must_inspect_result_
	SendKernelRoutinesOffsets(
		_In_ MPP_KERNEL_ROUTINES_OFFSETS KernelRoutines
	) {
		DWORD ReturnedBytes = 0x00;
		BOOL Success = ::DeviceIoControl(
			this->hDevice,
			MppInitialiseKernelRoutines(),
			&KernelRoutines,
			sizeof(MPP_KERNEL_ROUTINES_OFFSETS),
			nullptr,
			0x00,
			&ReturnedBytes,
			nullptr
		);
		if (!Success) {
			::printf("Failed to initialise kernel routines (%d).\r\n", ::GetLastError());
			return Success;
		}
	}

	/// @brief IOCTL to add a new image name.
	/// @param ImageName Structure used to add a new image name.
	BOOLEAN
	_Must_inspect_result_
	SendAddImageName(
		_In_ MPP_ADD_IMAGE_NAME ImageName
	) {
		DWORD ReturnedBytes = 0x00;
		BOOL Success = ::DeviceIoControl(
			this->hDevice,
			MppAddImageName(),
			&ImageName,
			sizeof(MPP_ADD_IMAGE_NAME),
			nullptr,
			0x00,
			&ReturnedBytes,
			nullptr
		);
		if (!Success) {
			::printf("Failed to add new image name (%d).\r\n", ::GetLastError());
			return Success;
		}
	}

private:
	/// @brief Handle to kernel device driver.
	HANDLE hDevice{ INVALID_HANDLE_VALUE };

};

#endif // !__MPPCLIENT_DEVICE_H_GUARD__


