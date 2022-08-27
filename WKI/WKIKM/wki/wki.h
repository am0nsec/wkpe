/*+================================================================================================
Module Name: wki.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wspe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Kernel Introspection (WKI).

================================================================================================+*/

#ifndef __WKI_H_GUARD__
#define __WKI_H_GUARD__

#ifndef _NTIFS_
#include <ntifs.h>
#endif // !_NTIFS_
#ifndef _NTSTRSAFE_H_INCLUDED_
#include <ntstrsafe.h>
#endif // !_NTSTRSAFE_H_INCLUDED_
#ifndef _AUX_KLIB_H
#include <aux_klib.h>
#endif // !_AUX_KLIB_H


// Windows Kernel Introspection (WKI) Memory Pool Tag - "Wki ".
#define WKI_MM_TAG  (ULONG)0x20696b57 

// Name of the Windows Registry key that store the current OS version.
#define WKI_CURRENTVERSION_KEY_NAME L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

// Name of the Windows Registry key that store all the WKI information.
#define WKI_KINTROSPECTION_KEY_NAME L"\\Registry\\Machine\\SOFTWARE\\KIntrospection"


/// <summary>
/// Structure representing a symbol.
/// </summary>
typedef struct _WKI_SYMBOL_ENTRY {
	LIST_ENTRY List;

	struct {
		UINT32 DJB;
		UINT32 OFF;
		UINT32 RVA;
		UINT32 SEG;
	} Body;
} WKI_SYMBOL_ENTRY, *PWKI_SYMBOL_ENTRY;


/// <summary>
/// Global data used internally by WKI.
/// </summary>
typedef struct _WKI_GLOBALS {
	UINT32 NumberOfSymbols;
	UINT32 NumberOfRoutines;
	UINT32 NumberOfStructures;

	UINT64 KernelBase;

	LIST_ENTRY SymbolHead;
	LIST_ENTRY StructureHead;
} WKI_GLOBALS, *PWKI_GLOBALS;


/// <summary>
/// 
/// </summary>
EXTERN_C NTSTATUS
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
WkiInitialise();


/// <summary>
/// 
/// </summary>
EXTERN_C VOID
_IRQL_requires_max_(APC_LEVEL)
WkiUninitialise();


/// <summary>
/// 
/// </summary>
/// <param name="SymbolName"></param>
/// <returns></returns>
EXTERN_C PVOID
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return != NULL)
WkiGetSymbol(
	_In_ LPCSTR SymbolName
);


EXTERN_C UINT64
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Success_(return != 0x00)
WkiReadValue(
	_In_ PVOID  Address,
	_In_ UINT16 Size
);

// Global Windows Kernel Introspection (WKI) Data.
extern WKI_GLOBALS WkiGlobal;

#endif // !__WKI_H_GUARD__
