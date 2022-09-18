/*+================================================================================================
Module Name: worker.hpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Memory Patching Protection (MPP) Worker for Image .text section protection.
================================================================================================+*/

#ifndef __MPP_WORKER_H_GUARD__
#define __MPP_WORKER_H_GUARD__

#include "mpp.hpp"
#include "worker.hpp"

/// @brief System worker information
namespace MppWorker {

	/// @brief Work item to protect .text section of a module.
	extern PIO_WORKITEM ProtectWorker;

	/// @brief Image Loading callback registered via PsSetLoadImageNotifyRoutine.
	/// @param FullImageName Name of the image loaded from the system.
	/// @param ProcessId     Current process unique ID.
	/// @param ImageInfo     Further information about the image being loaded.
	VOID __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(PASSIVE_LEVEL)
	_IRQL_requires_max_(PASSIVE_LEVEL)
	ProtectWorkerCallback(
		_In_     PDEVICE_OBJECT DeviceObject,
		_In_opt_ PVOID          WorkerData
	);


	/// @brief Get the start and end address of the .text section of an image
	/// @param ImageBaseAddress Base address of the image being loaded.
	/// @param AddressStart     Start address.
	/// @param AddressEnd       End address.
	VOID __declspec(code_seg("PAGE"))
	_IRQL_requires_min_(PASSIVE_LEVEL)
	_IRQL_requires_max_(PASSIVE_LEVEL)
	GetImageTextSectionAddresses(
		_In_  PVOID  ImageBaseAddress,
		_Out_ PVOID* AddressStart,
		_Out_ PVOID* AddressEnd
	);

	/// @brief Check if section name is ".text"
	/// @param x Section name array.
	constexpr bool IsTextSection(UCHAR x[0x08]) {
		return (
			x[0x00] == '.'
			&& x[0x01] == 't'
			&& x[0x02] == 'e'
			&& x[0x03] == 'x'
			&& x[0x04] == 't'
		);
	}

	/// @brief Data Context for the protect worker.
	typedef struct _MPP_WORKER_PROTECT_DATA {
		HANDLE ThreadId;
		HANDLE ProcessId;
		PVOID  ImageBaseAddress;
	} MPP_WORKER_PROTECT_DATA, *PMPP_WORKER_PROTECT_DATA;
}

#endif // !__MPP_WORKER_H_GUARD__
