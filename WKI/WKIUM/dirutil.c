/*+================================================================================================
Module Name: dirutil.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Directory utility code.
Used to change directory to the "PDB" folder and get the correct symbol server search path.

================================================================================================+*/

#include <Windows.h>
#include <strsafe.h>
#include <winternl.h>

#include "inc/dirutil.h"


_Use_decl_annotations_
NTSTATUS GetSymSrvSearchPath(
	_Out_ PWCHAR* SymSearchPath
) {
	// Get the current path
	PWSTR DefaultDirectory[MAX_PATH] = { 0x00 };
	DWORD dwCurrentDirectory = GetCurrentDirectoryW(MAX_PATH, (LPWSTR)DefaultDirectory);
	if (dwCurrentDirectory == 0x00)
		return E_FAIL;
	
	// Other local Stack Variables
	CONST PWCHAR Pdb = L"pdb";
	CONST PWCHAR Sym = L"symsrv";
	CONST PWCHAR Dll = L"symsrv.dll";
	CONST PWCHAR Web = L"https://msdl.microsoft.com/download/symbols";

	// Calculate the size of the buffer to allocate
	DWORD dwBuffer = 0x100;
	DWORD dwSize   = lstrlenW(Pdb) + lstrlenW(Sym) + lstrlenW(Dll) + lstrlenW(Web) + (sizeof(WCHAR) * 4) + lstrlenW((LPWSTR)DefaultDirectory);
	while (dwBuffer < dwSize)
		dwBuffer += 0x100;

	PWCHAR Out = calloc(1, dwBuffer);
	if (Out == NULL) {
		*SymSearchPath = NULL;
		return E_OUTOFMEMORY;
	}

	// Assemble the whole string now
	HRESULT Result = StringCbPrintfW(
		Out,
		dwBuffer,
		L"%s*%s*%s\\%s*%s\0",
		Sym,
		Dll,
		(LPWSTR)DefaultDirectory,
		Pdb,
		Web
	);
	if (FAILED(Result)) {
		free(Out);
		*SymSearchPath = NULL;
		return E_FAIL;
	}

	*SymSearchPath = Out;
	return S_OK;
}
