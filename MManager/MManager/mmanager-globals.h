/*+================================================================================================
Module Name: mmanager-globals.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
This file contains all global definition and information used by the kernel driver.

================================================================================================+*/

#ifndef __MMANAGER_GLOBALS_H_GUARD__
#define __MMANAGER_GLOBALS_H_GUARD__

#include <ntifs.h>
#include <ntddk.h>

// Include security routines for driver
#include <wdmsec.h>
#pragma comment(lib, "wdmsec.lib")

// MManager Device driver Memory Pool Tag - "MMan"
#define MMANAGER_MM_TAG (ULONG)0x6e614d4d

// MManager Device driver name
#define MMANAGER_DEVICE_NAME L"\\Device\\MManager"

// MManager Device driver symbolic name
#define MMANAGER_SYMBOLIC_LINK_NAME L"\\??\\MManager"

// MManager Debug tag
#define MMANAGER_DEBUG_TAG "[MManager] "

// MManager wrapper around KdPrint for less bloated code.
#define MMDebug(_x_) KdPrint((MMANAGER_DEBUG_TAG)); KdPrint(_x_)

// MManager class GUID - {F7BD4AF3-BA07-4EB4-BD62-4BBF596D5C23}
// {5C368CE1-3DB6-43EE-8EA8-9338B4803FAE}
static CONST GUID MMANAGER_CLASS_GUID = { 
	0x5c368ce1,
	0x3db6,
	0x43ee,
	{ 0x8e, 0xa8, 0x93, 0x38, 0xb4, 0x80, 0x3f, 0xae }
};

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

#endif // !__MMANAGER_GLOBALS_H_GUARD__
