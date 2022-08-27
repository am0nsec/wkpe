/*+================================================================================================
Module Name: registry.c
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Registry related routines. Used to create and store symbols for access from the kernel driver.
================================================================================================+*/
#include "inc/registry.h"

// Handle to the base registry key
HKEY g_BaseKey = INVALID_HANDLE_VALUE;

// Handle to the key for the current build number
HKEY g_BuilKey = INVALID_HANDLE_VALUE;

// Handle to the key for the public symbols
HKEY g_Symbols = INVALID_HANDLE_VALUE;

// Handle to the key for the structures
HKEY g_Structs = INVALID_HANDLE_VALUE;

// Handle to the key for the routines
HKEY g_Routines = INVALID_HANDLE_VALUE;

// The OS version string
WCHAR g_VersionString[0x20] = { 0x00 };


/// <summary>
/// Display message related to an error message.
/// </summary>
/// <param name="dwError">Error id returned by GetLastError.</param>
VOID RtlGetErrorMessageW(
	_In_ DWORD dwError
) {
	WCHAR ErrorW[1024] = { 0x00 };
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0x00, ErrorW, 1024, NULL) != 0x0) {
		wprintf(L"Message is \"%s\"\n", ErrorW);
	}
	else {
		wprintf(L"Failed! Error code %d\n", GetLastError());
	}
}


/// <summary>
/// Get the OS build number.
/// </summary>
HRESULT RtlGetOSVersion() {
	LSTATUS Status = ERROR_SUCCESS;

	// Get the major build version
	DWORD CurrentMajorVersionNumber = 0x00;
	DWORD CurrentMajorVersionNumberSize = sizeof(DWORD);
	Status = RegGetValueA(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		"CurrentMajorVersionNumber",
		RRF_RT_REG_DWORD,
		NULL,
		&CurrentMajorVersionNumber,
		&CurrentMajorVersionNumberSize
	);
	if (Status != ERROR_SUCCESS) {
		RtlGetErrorMessageW((DWORD)GetLastError());
		return E_FAIL;
	}

	// Get the current build number
	char  CurrentBuildNumber[0x10] = { 0x00 };
	DWORD CurrentBuildNumberSize = ARRAYSIZE(CurrentBuildNumber);
	Status = RegGetValueA(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		"CurrentBuildNumber",
		RRF_RT_REG_SZ,
		NULL,
		CurrentBuildNumber,
		&CurrentBuildNumberSize
	);
	if (Status != ERROR_SUCCESS) {
		RtlGetErrorMessageW((DWORD)GetLastError());
		return E_FAIL;
	}

	// Get the update build revision
	DWORD UpdateBuildRevision = 0x00;
	DWORD UpdateBuildRevisionSize = sizeof(DWORD);
	Status = RegGetValueA(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		"UBR",
		RRF_RT_REG_DWORD,
		NULL,
		&UpdateBuildRevision,
		&UpdateBuildRevisionSize
	);
	if (Status != ERROR_SUCCESS) {
		RtlGetErrorMessageW((DWORD)GetLastError());
		return E_FAIL;
	}

	// Build the version string
	HRESULT Result = StringCbPrintfW(
		g_VersionString,
		sizeof(WCHAR) * 0x20,
		L"%u.%c%c%c%c%c.%u\0",
		CurrentMajorVersionNumber,
		CurrentBuildNumber[0],
		CurrentBuildNumber[1],
		CurrentBuildNumber[2],
		CurrentBuildNumber[3],
		CurrentBuildNumber[4],
		UpdateBuildRevision
	);
	return Result;
}


/// <summary>
/// Get the DJBT hash of a string.
/// </summary>
DWORD RtlGetHash(BSTR String) {
	// Allocate memory
	LPSTR MultiByte = calloc(0x01, 0x100);
	if (MultiByte == NULL)
		return 0x00;

	// Convert from wide-character to  multibyte-character
	SIZE_T Count = 0x00;
	wcstombs_s(&Count, MultiByte, 0x100, String, lstrlenW(String));
	if (Count == 0x00) {
		free(MultiByte);
		return 0x00;
	}

	// Calculate the djb2 hash
	DWORD Hash = 0x1505;
	INT   c    = 0x0000;
	LPSTR d    = MultiByte;

	while (c = *d++)
		Hash = ((Hash << 5) + Hash) + c;

	free(MultiByte);
	return Hash;
}


_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE RegistryInitialise(
	VOID
) {
	// Check if already initialised
	if (g_BaseKey != INVALID_HANDLE_VALUE)
		return S_OK;

	// Check if key exist
	LSTATUS Status = ERROR_SUCCESS;
	RtlCreateOrOpenKey(Status, HKEY_LOCAL_MACHINE, REGISTRY_BASE_KEY, &g_BaseKey);

	// Get OS Version
	if (FAILED(RtlGetOSVersion())) {
		RtlGetErrorMessageW((DWORD)GetLastError());
		return E_FAIL;
	}
	wprintf(L"[*] OS Build: %s\r\n", g_VersionString);
	
	// Open or create the key with the version string
	RtlCreateOrOpenKey(Status, g_BaseKey, g_VersionString, &g_BuilKey);
	
	// Open or create the keys for:
	//   - Public Symbols 
	//   - Structures
	//   - Routines
	RtlCreateOrOpenKey(Status, g_BuilKey, REGISTRY_SYMBOLS_KEY,  &g_Symbols);
	RtlCreateOrOpenKey(Status, g_BuilKey, REGISTRY_STRUCTS_KEY,  &g_Structs);
	RtlCreateOrOpenKey(Status, g_BuilKey, REGISTRY_ROUTINES_KEY, &g_Routines);

	return S_OK;
}


_Use_decl_annotations_
VOID STDMETHODCALLTYPE RegistryUninitialise(
	VOID
) {
	if (g_BaseKey != INVALID_HANDLE_VALUE)
		RegCloseKey(g_BaseKey);
	if (g_BuilKey != INVALID_HANDLE_VALUE)
		RegCloseKey(g_BuilKey);
	if (g_Symbols != INVALID_HANDLE_VALUE)
		RegCloseKey(g_Symbols);
	if (g_Structs != INVALID_HANDLE_VALUE)
		RegCloseKey(g_Structs);
	if (g_Routines != INVALID_HANDLE_VALUE)
		RegCloseKey(g_Routines);

	memset(g_VersionString, 0x00, 0x20);
}


_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE RegistryAddSymbols(
	_In_ PUBLIC_SYMBOL Symbols[],
	_In_ DWORD         Entries
) {
	if (g_Symbols == INVALID_HANDLE_VALUE || Entries == 0x00 || Symbols == NULL)
		return E_FAIL;

	// Parse all the entries
	DWORD ValidEntries = 0x00;
	for (DWORD cx = 0x00; cx < Entries; cx++) {

		// If symbol has not been resolved, go to next entry
		if (Symbols[cx].dwRVA == 0x00)
			continue;
		ValidEntries++;

		// Create new key
		HKEY    CurrentKey = INVALID_HANDLE_VALUE;
		LSTATUS Status     = ERROR_SUCCESS;
		RtlCreateOrOpenKey(Status, g_Symbols, Symbols[cx].Name, &CurrentKey);
		wprintf(L"\\HKLM\\%s\\%s\\%s\\%s\r\n", REGISTRY_BASE_KEY, g_VersionString, REGISTRY_SYMBOLS_KEY, Symbols[cx].Name);

		// Add information
		RegSetKeyValueW(CurrentKey, NULL, L"RVA", REG_DWORD, &Symbols[cx].dwRVA, sizeof(DWORD));
		RegSetKeyValueW(CurrentKey, NULL, L"OFF", REG_DWORD, &Symbols[cx].dwOff, sizeof(DWORD));
		RegSetKeyValueW(CurrentKey, NULL, L"SEG", REG_DWORD, &Symbols[cx].dwSeg, sizeof(DWORD));

		// Get the hash of the name
		DWORD Hash = RtlGetHash(Symbols[cx].Name);
		RegSetKeyValueW(CurrentKey, NULL, L"DJB", REG_DWORD, &Hash, sizeof(DWORD));
		
		RegCloseKey(CurrentKey);
	}

	// Set the total number of entries
	RegSetKeyValueW(g_BuilKey, NULL, L"NumberOfSymbols", REG_DWORD, &ValidEntries, sizeof(DWORD));
	return S_OK;
}