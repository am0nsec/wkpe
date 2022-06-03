/*+================================================================================================
Module Name: main.cpp
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.


Abstract:
Console application entry point.

================================================================================================+*/

#include <Windows.h>
#include <memory>
#include <stdio.h>

#include "mmanager.h"

INT32 main(
	_In_ int         argc,
	_In_ const char* argv[]
) {
	// Banner
	wprintf(L"================================================================================================\r\n");
	wprintf(L"Module Name: Virtual Address Descriptor List (vadlist)                                          \r\n");
	wprintf(L"Author     : Paul L. (@am0nsec)                                                                 \r\n");
	wprintf(L"Origin     : https://github.com/am0nsec/wkpe/                                                   \r\n\r\n");
	wprintf(L"Tested OS  : Windows 10 (20h2) - 19044.1706                                                     \r\n");
	wprintf(L"================================================================================================\r\n\r\n");
	
	// Check for parameters
	if (argc < 0x02) {
		printf("Usage: vadlist.exe <process id>\r\n\r\n");
		return EXIT_FAILURE;
	}

	// Check for the PID provided
	ULONG ProcessId = atoi(argv[0x01]);;
	if (ProcessId <= 0x04 || (ProcessId % 4) != 0x00) {
		printf("Invalid process ID supplied. \r\n\r\n");
		return EXIT_FAILURE;
	}

	// Open handle to the device driver
	std::unique_ptr<CMManager> MManager = std::make_unique<CMManager>();
	if (!MManager->IsDeviceReady()) {
		printf("Failed to open handle to device driver.\r\n\r\n");
		return EXIT_FAILURE;
	}
	
	// Get the list of VADs
	if (!MManager->FindProcessVads(ProcessId)) {
		printf("Failed to retrieve VAD list.\r\n\r\n");
		return EXIT_FAILURE;
	}
	MManager->PrintProcessVads();

	return EXIT_SUCCESS;
}
