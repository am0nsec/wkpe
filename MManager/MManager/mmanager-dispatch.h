/*+================================================================================================
Module Name: mmanager-dispatch.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
This file contains all global definition and information used by the kernel driver.

================================================================================================+*/

#ifndef __MMANAGER_DISPATCH_H_GUARD__
#define __MMANAGER_DISPATCH_H_GUARD__

#include "mmanager-globals.h"

/// <summary>
/// The Unload routine performs any operations that are necessary before the system unloads the driver.
/// </summary>
/// <param name="DriverObject">Caller-supplied pointer to a DRIVER_OBJECT structure. This is the driver's driver object.</param>
_IRQL_requires_max_(PASSIVE_LEVEL)
EXTERN_C VOID
MmanDriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
);

/// <summary>
/// The callback routine for IRP_MJ_CREATE and IRP_MJ_CLOSE.
/// </summary>
/// <param name="DeviceObject">Caller-supplied pointer to a DEVICE_OBJECT structure.This is the device object for the target device, previously created by the driver's AddDevice routine.</param>
/// <param name="Irp">Caller-supplied pointer to an IRP structure that describes the requested I/O operation.</param>
/// <returns>If the routine succeeds, it must return STATUS_SUCCESS. Otherwise, it must return one of the error status values defined in Ntstatus.h.</returns>
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
_IRQL_requires_max_(PASSIVE_LEVEL)
EXTERN_C NTSTATUS
MmanDriverCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
);

/// <summary>
/// The callback routine services various IRPs. In this case handle user-mode IOCTL. For a list of function codes, see mmanager-globals.h.
/// </summary>
/// <param name="DeviceObject">Caller-supplied pointer to a DEVICE_OBJECT structure.This is the device object for the target device, previously created by the driver's AddDevice routine.</param>
/// <param name="Irp">Caller-supplied pointer to an IRP structure that describes the requested I/O operation.</param>
/// <returns>If the routine succeeds, it must return STATUS_SUCCESS. Otherwise, it must return one of the error status values defined in Ntstatus.h.</returns>
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C NTSTATUS
MmanDriverDispatch(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP           Irp
);

#endif // !__MMANAGER_DISPATCH_H_GUARD__
