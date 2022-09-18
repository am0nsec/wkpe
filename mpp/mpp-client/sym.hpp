/*+================================================================================================
Module Name: sym.hpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows PDB symbols Extractor.
Based on: https://github.com/Coldzer0/TinyPDBParser
================================================================================================+*/

#ifndef __MPPCLIENT_SYM_H_GUARD__
#define __MPPCLIENT_SYM_H_GUARD__

#pragma comment(lib, "urlmon")
#pragma comment(lib, "imagehlp")

#include <windows.h>
#include <imagehlp.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <map>


/// @brief CodeView header
typedef struct _CV_HEADER {
	DWORD CvSignature;// NBxx
	LONG Offset;      // Always 0 for NB10
} CV_HEADER, * PCV_HEADER;

/// @brief CodeView NB10 debug information.
/// (used when debug information is stored in a PDB 2.00 file)
typedef struct _CV_INFO_PDB20 {
	CV_HEADER Header;
	DWORD Signature;    // seconds since 01.01.1970
	DWORD Age;          // an always-incrementing value
	BYTE PdbFileName[1];// zero terminated string with the name of the PDB file
} CV_INFO_PDB20, * PCV_INFO_PDB20;


/// @brief CodeView RSDS debug information.
/// (used when debug information is stored in a PDB 7.00 file)
typedef struct _CV_INFO_PDB70 {
	DWORD CvSignature;
	GUID Signature;     // unique identifier
	DWORD Age;          // an always-incrementing value
	BYTE PdbFileName[1];// zero terminated string with the name of the PDB file
} CV_INFO_PDB70, * PCV_INFO_PDB70;

/// @brief PDB 7.00 CodeView Signature - 'SDSR'
constexpr DWORD PDB70 = 0x53445352;

/// @brief PDB 2.00 CodeView Signature - '01BN'
constexpr DWORD PDB20 = 0x3031424e;

/// @brief 
class CSymDatabase {
public:
	CSymDatabase(
		_In_ LPCSTR FilePath
	) {
		this->FilePath = std::string(FilePath);

		// Get current directory
		char Buffer[MAX_PATH] = { 0x00 };
		::GetCurrentDirectoryA(MAX_PATH, Buffer);	
		this->CurrentDirectory = Buffer;
	}

	/// @brief Download the PDB from Microsoft symbol server for ntoskrnl.exe executable file.
	HRESULT
	_Must_inspect_result_
	DownloadFromServer(
		VOID
	);

	/// @brief Initialise, load and set callback to enumerate symbols from PDB file.
	HRESULT
	_Must_inspect_result_
	LoadAndCheckSym(
		VOID
	);

public:
	/// @brief Key value pair of symbols and their offsets.
	std::map<std::string, DWORD> Symbols;

private:

	/// @brief Get the data directory from ntoskrnl.exe executable file.
	/// @param ModuleBase Base address in memory of the ntoskrnl.exe executable file.
	PIMAGE_DATA_DIRECTORY
	_Must_inspect_result_
	GetImageDataDirectory(
		_In_ LPVOID ModuleBase
	);

	/// @brief Calculate virtual address for an offset and base address in memory of the ntoskrnl.exe executable file.
	/// @param ModuleBase      Base address in memory of the ntoskrnl.exe executable file.
	/// @param RelativeAddress Offset of the data to get.
	DWORD
	_Must_inspect_result_
	GetVirtualAddress(
		_In_ LPVOID ModuleBase,
		_In_ DWORD  RelativeAddress
	);

	/// @brief Convert a GUID to a string.
	/// @param Guid GUID to convert to string.
	_inline std::string
	_Must_inspect_result_
	GuidToString(
		_In_ GUID* Guid
	);

	/// @brief Callback to go through all symbols from the PDB file.
	/// @param pSymInfo    Pointer to symbol information structure.
	/// @param SymbolSize  Size of symbol.
	/// @param UserContext Arbitrary user context.
	/// @return 
	static BOOLEAN
	_Must_inspect_result_
	EnumSymCallback(
		_In_ PSYMBOL_INFO pSymInfo,
		_In_ ULONG        SymbolSize,
		_In_ PVOID        UserContext
	);


private:
	/// @brief Path to rhe ntoskrnl.exe executable file on disk.
	std::string FilePath;

	/// @brief Path to the PDB file on disk post-download. 
	std::string OutputPath;

	/// @brief Path to the current working directory.
	std::string CurrentDirectory;

	/// @brief Web URL of the PDB file from the Microsoft symbol server.
	std::string ServerPath;

	/// @brief Name of the PDB file.
	std::string PdbName;
};


#endif // !__MPPCLIENT_SYM_H_GUARD__
