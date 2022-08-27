/*+================================================================================================
Module Name: ki-globals.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
This file contains all global definition and information used by the kernel driver.
================================================================================================+*/

#ifndef __KI_H_GUARD__
#define __KI_H_GUARD__

#include <ntifs.h>
#include <ntddk.h>

// Include security routines for driver
#include <wdmsec.h>

// Windows Kernel Introspection Device driver name
#define KI_DEVICE_NAME L"\\Device\\WKI"

// Windows Kernel Introspection Device driver symbolic name
#define KI_SYMBOLIC_LINK_NAME L"\\??\\WKI"

// Windows Kernel Introspection Debug tag
#define KI_DEBUG_TAG "[WKI] "

// Windows Kernel Introspection wrapper around KdPrint for less bloated code.
#define KiDebug(_x_) KdPrint((KI_DEBUG_TAG)); KdPrint(_x_)

// Windows Kernel Introspection class GUID - {3E07100B-E36E-4F2E-8345-1399B9947497}
static CONST GUID KI_CLASS_GUID = {
	0x3e07100b,
	0xe36e,
	0x4f2e,
	{ 0x83, 0x45, 0x13, 0x99, 0xb9, 0x94, 0x74, 0x97 }
};

#endif // !__KI_H_GUARD__
