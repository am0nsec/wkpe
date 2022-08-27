/*+================================================================================================
Module Name: registry.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Registry related routines. Used to create and store symbols for access from the kernel driver.
================================================================================================+*/

#ifndef __KI_REGISTRY_H_GUARD__
#define __KI_REGISTRY_H_GUARD__

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>

#include "interface.h"

// Base registry key
#define REGISTRY_BASE_KEY     L"SOFTWARE\\WKI"

#define REGISTRY_ROUTINES_KEY L"Routines"
#define REGISTRY_SYMBOLS_KEY  L"Symbols"
#define REGISTRY_STRUCTS_KEY  L"Structs"

// Dodgy macro to make the code less bloated
#define RtlCreateOrOpenKey(Status, hKey, SubKey, hResult) \
	Status = RegCreateKeyExW(hKey, SubKey, 0x00, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, hResult, NULL); \
	if (Status != ERROR_SUCCESS) { RtlGetErrorMessageW((DWORD)Status); RegistryUninitialise(); return E_FAIL; }

/// <summary>
/// Get OS Build number and initialise Windows Registry keys.
/// </summary>
HRESULT STDMETHODCALLTYPE
_Must_inspect_result_
_Success_(return == S_OK) RegistryInitialise(
	VOID
);


/// <summary>
/// Internal cleanup of all handles and memory.
/// </summary>
VOID STDMETHODCALLTYPE RegistryUninitialise(
	VOID
);


/// <summary>
/// Add symbols to the windows registry.
/// </summary>
HRESULT STDMETHODCALLTYPE RegistryAddSymbols(
	_In_ PUBLIC_SYMBOL Symbols[],
	_In_ DWORD         Entries
);

#endif // !__KI_REGISTRY_H_GUARD__
