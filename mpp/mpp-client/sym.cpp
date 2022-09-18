/*+================================================================================================
Module Name: sym.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows PDB symbols Extractor.
Based on: https://github.com/Coldzer0/TinyPDBParser
================================================================================================+*/

#include "sym.hpp"


_Use_decl_annotations_
HRESULT CSymDatabase::DownloadFromServer(
	VOID
) {

	// Make sure path exists
	if (!std::filesystem::exists(this->FilePath)) {
		::printf("File does not exist: %s\r\n", this->FilePath.c_str());
		return E_UNEXPECTED;
	}

	// Get the content of the EXE file
	std::ifstream Executable(
		this->FilePath,
		std::ios::binary | std::ios::ate
	);
	SIZE_T ExecutableSize = Executable.tellg();

	// Set pointer to beginning of file
	Executable.seekg(0x00, std::ios::beg);
	
	// Allocate memory
	PVOID ModuleBase = ::malloc(ExecutableSize);
	if (ModuleBase == nullptr) {
		::printf("Unable to allocate memory for executable: %d\r\n", ::GetLastError());
		return E_OUTOFMEMORY;
	}

	// Start reading the file
	if (!Executable.read((char*)ModuleBase, ExecutableSize)) {
		::free(ModuleBase);
		::printf("Unable to read content of the file\r\n");
		return E_UNEXPECTED;
	}

	// Get debug directory
	PIMAGE_DATA_DIRECTORY DataDirectory = this->GetImageDataDirectory(ModuleBase);
	if (DataDirectory == nullptr) {
		::free(ModuleBase);
		::printf("Failed to get debug directory for executable\r\n");
		return E_UNEXPECTED;
	}

	// Get offset to the data within the executable
	DWORD Offset = this->GetVirtualAddress(
		ModuleBase,
		DataDirectory->VirtualAddress
	);
	if (Offset == 0x00) {
		::free(ModuleBase);
		::printf("Failed to get debug directory for executable\r\n");
		return E_UNEXPECTED;
	}

	// Parse all debug entries
	PIMAGE_DEBUG_DIRECTORY DebugEntry = reinterpret_cast<PIMAGE_DEBUG_DIRECTORY>(
		reinterpret_cast<LPBYTE>(ModuleBase)
		+ Offset
	); 
	for (DWORD cx = 0x00; cx < (DataDirectory->Size / sizeof(IMAGE_DEBUG_DIRECTORY)); cx++) {

		// Check if the type of debug is contains what is required
		if (DebugEntry->Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
			DebugEntry++;
			continue;
		}

		// Get the CodeView Header
		PCV_HEADER CvHeader = reinterpret_cast<PCV_HEADER>(
			reinterpret_cast<LPBYTE>(ModuleBase)
			+ DebugEntry->PointerToRawData
		);

		// Check if PDB 2.00 or PDB 7.00
		switch (CvHeader->CvSignature) {
			case PDB70: {
				PCV_INFO_PDB70 Info = reinterpret_cast<PCV_INFO_PDB70>(CvHeader);

				// Get basic information
				this->PdbName   = reinterpret_cast<LPSTR>(Info->PdbFileName);

				// Build the server URL to the PDB
				this->ServerPath =
					std::string("http://msdl.microsoft.com/download/symbols/")
					+ this->PdbName
					+ "/"
					+ this->GuidToString(&Info->Signature)
					+ std::to_string(Info->Age)
					+ "/"
					+ this->PdbName;
				break;
			}
			case PDB20: {
				PCV_INFO_PDB20 Info = reinterpret_cast<PCV_INFO_PDB20>(CvHeader);

				// Get basic information
				this->PdbName = reinterpret_cast<LPSTR>(Info->PdbFileName);

				// Build the server URL to the PDB
				char AgeString[10];
				::snprintf(AgeString, sizeof(AgeString), "%08X%x", Info->Signature, Info->Age);

				this->ServerPath =
					std::string("http://msdl.microsoft.com/download/symbols/")
					+ this->PdbName
					+ "/"
					+ std::string(AgeString)
					+ "/"
					+ this->PdbName;

				break;
			}
			default: {
				::free(ModuleBase);
				::printf("Unsupported CodeView signature %d.\n\r", CvHeader->CvSignature);
				return E_UNEXPECTED;
			}
		}

		// Display Information
		::printf("[+] PDB Information\r\n");
		::printf("    %s\r\n", this->ServerPath.c_str());
		::free(ModuleBase);

		// Update the 
		this->OutputPath =
			this->CurrentDirectory + "\\" + this->PdbName;

		// Download file
		HRESULT hr = URLDownloadToFileA(
			nullptr,
			this->ServerPath.c_str(),
			this->OutputPath.c_str(),
			BINDF_GETNEWESTVERSION,
			nullptr
		);
		if (FAILED(hr))
			::printf("[-] Failed to download PDB from symbol server: %d\r\n", ::GetLastError());
		else
			::printf("[+] PDB successfully downloaded from symbol server.\r\n");

		return hr;
	}

	// Cleanup and exit
	if (ModuleBase != nullptr)
		::free(ModuleBase);
	return E_UNEXPECTED;
}


_Use_decl_annotations_
HRESULT CSymDatabase::LoadAndCheckSym(
	VOID
) {

	// Open handle to current process
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
	if (hProcess == INVALID_HANDLE_VALUE) {
		::printf("Unable to open handle to current process: %d\r\n", ::GetLastError());
		return E_UNEXPECTED;
	}

	// Initialise symbol library
	bool SymbolsPathInit = ::SymInitialize(
		hProcess,
		this->OutputPath.c_str(),
		false
	);
	if (!SymbolsPathInit) {
		::printf("Unable to initialise symbol library: %d\r\n", ::GetLastError());

		::CloseHandle(hProcess);
		return E_UNEXPECTED;
	}

	// Load PDB file
	::SymSetOptions(SymGetOptions() | SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_ANYTHING);
	::SymSetSearchPath(hProcess, this->OutputPath.c_str());

	SIZE_T SymBase = ::SymLoadModule(
		hProcess,
		nullptr,
		"C:\\Windows\\System32\\ntoskrnl.exe",
		nullptr,
		0x00,
		0x00
	);
	if (SymBase == 0x00) {
		::printf("Failed to load module: %d\r\n", ::GetLastError());

		::CloseHandle(hProcess);
		return E_UNEXPECTED;
	}

	// Set symbol enumeration callback
	HRESULT hr = S_OK;
	BOOL Success = SymEnumSymbols(
		hProcess,
		SymBase,
		nullptr,
		(PSYM_ENUMERATESYMBOLS_CALLBACK)CSymDatabase::EnumSymCallback,
		this
	);
	if (!Success) {
		::printf("Failed to set symbol enumeration callback: %d\r\n", ::GetLastError());

		::CloseHandle(hProcess);
		return E_UNEXPECTED;
	}
	
	// Cleanup and return
	::CloseHandle(hProcess);
	return S_OK;
}


_Use_decl_annotations_
PIMAGE_DATA_DIRECTORY CSymDatabase::GetImageDataDirectory(
	_In_ LPVOID ModuleBase
) {
	if (ModuleBase == nullptr)
		return nullptr;

	// DOS Header
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	// NT Header
	PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<PBYTE>(ModuleBase)
		+ DosHeader->e_lfanew
	);
	if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	// Get data directory
	if (!NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64
		|| !NtHeaders->FileHeader.Machine == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		::printf("Unsupported executable machine type: %d\r\n", NtHeaders->FileHeader.Machine);
		return nullptr;
	}
	
	return &NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
}


_Use_decl_annotations_
DWORD CSymDatabase::GetVirtualAddress(
	_In_ LPVOID ModuleBase,
	_In_ DWORD  RelativeAddress
) {
	// DOS Header
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return 0x00;

	// NT Header
	PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<PBYTE>(ModuleBase)
		+ DosHeader->e_lfanew
		);
	if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
		return 0x00;

	PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(NtHeaders);
	for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; ++i) {
		if (RelativeAddress >= Section->VirtualAddress
			&& RelativeAddress < (Section->VirtualAddress + Section->SizeOfRawData)) {
			return (Section->PointerToRawData + (RelativeAddress - Section->VirtualAddress));
		}
			
		Section++;
	}
	return 0x00;
}


_Use_decl_annotations_
_inline std::string CSymDatabase::GuidToString(
	_In_ GUID* Guid
) {
	char Str[37]; // 32 hex chars + 4 hyphens + null terminator
	snprintf(Str, sizeof(Str),
		"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
		Guid->Data1,
		Guid->Data2,
		Guid->Data3,
		Guid->Data4[0],
		Guid->Data4[1],
		Guid->Data4[2],
		Guid->Data4[3],
		Guid->Data4[4],
		Guid->Data4[5],
		Guid->Data4[6],
		Guid->Data4[7]
	);
	return Str;
}


_Use_decl_annotations_
BOOLEAN CSymDatabase::EnumSymCallback(
	_In_ PSYMBOL_INFO pSymInfo,
	_In_ ULONG        SymbolSize,
	_In_ PVOID        UserContext
) {
	if (pSymInfo->NameLen == 0)
		return true;
	auto This = reinterpret_cast<CSymDatabase*>(UserContext);

	for (auto& kvp : This->Symbols) {
		if (std::string(pSymInfo->Name).find(kvp.first) == 0
			&& ::strlen(pSymInfo->Name) == kvp.first.length()
		) {
			kvp.second = pSymInfo->Address - pSymInfo->ModBase;
			printf("   - 0x%x : %s\r\n",
				(DWORD)(pSymInfo->Address - pSymInfo->ModBase),
				(LPSTR)pSymInfo->Name
			);
		}
	}

	return true;
}