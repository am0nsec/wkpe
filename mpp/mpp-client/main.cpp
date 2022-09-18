/*+================================================================================================
Module Name: main.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Memory Patching Protection (MPP) Kernel Device Driver User-Mode Client.
================================================================================================+*/


#include <Windows.h>
#include <iostream>
#include <memory>
#include <stdio.h>

#include "sym.hpp"
#include "device.hpp"


INT32 main(
	_In_ int         argc,
	_In_ const char* argv[]
) {
	// Banner
	wprintf(L"=================================================================\r\n");
	wprintf(L"Module Name: Memory Patching Protection (MPP) User-Mode Client   \r\n");
	wprintf(L"Author     : Paul L. (@am0nsec)                                  \r\n");
	wprintf(L"Origin     : https://github.com/am0nsec/wkpe/                    \r\n");
	wprintf(L"Tested OS  : Windows 10 (20h2) - 19044.2006                      \r\n");
	wprintf(L"=================================================================\r\n");

	// Download PDB file of ntoskrnl
	auto SymDatabase = std::make_unique<CSymDatabase>("C:\\Windows\\System32\\ntoskrnl.exe");

	if (FAILED(SymDatabase->DownloadFromServer()))
		return EXIT_FAILURE;

	// Add symbols to find
	SymDatabase->Symbols.insert(std::make_pair("MiAddSecureEntry", 0x00));
	SymDatabase->Symbols.insert(std::make_pair("MiObtainReferencedVadEx", 0x00));
	SymDatabase->Symbols.insert(std::make_pair("MiUnlockAndDereferenceVad", 0x00));

	// Find symbols
	::printf("\r\n[+] Searching symbols ...\r\n");
	if (FAILED(SymDatabase->LoadAndCheckSym()))
		return EXIT_FAILURE;
	::printf("[+] Searching symbols ... OK\r\n");

	// Send information to the kernel driver
	auto DeviceManager = std::make_unique<CDeviceManager>();
	if (!DeviceManager->IsDeviceReady()) {
		::printf("[-] Failed to open handle to kernel device.\r\n");
		return EXIT_FAILURE;
	}

	MPP_KERNEL_ROUTINES_OFFSETS KernelRoutines = {
		.MiAddSecureEntry          = SymDatabase->Symbols["MiAddSecureEntry"],
		.MiObtainReferencedVadEx   = SymDatabase->Symbols["MiObtainReferencedVadEx"],
		.MiUnlockAndDereferenceVad = SymDatabase->Symbols["MiUnlockAndDereferenceVad"]
	};
	DeviceManager->SendKernelRoutinesOffsets(KernelRoutines);

	// Add AMSI DLL
	MPP_ADD_IMAGE_NAME ImageName = { 
		.Size = ARRAYSIZE(AmsiImageName) * sizeof(WCHAR)
	};
	RtlCopyMemory(ImageName.Name, AmsiImageName, ImageName.Size);
	DeviceManager->SendAddImageName(ImageName);

	return EXIT_SUCCESS;
}