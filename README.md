Windows Kernel Programming Experiments
--------------------------------------
All projects and the code within this repository are solely proof of concepts and have not been thoroughly tested on different versions of Microsoft Windows.

The `DriverEntry` routines of each driver checks for the version of the operating system and will make sure it is `Windows 10 (20h2) - 19044.1706`, as it the Windows 10 version I used to test the drivers.

All structures and other `typedef` have been defined via available PDBs, WinDBG and [resym tool](https://github.com/ergrelet/resym). Structures and data may differ from one version to another - use with caution. 

### MManager
Experiments with the Windows Memory Manager (Mm/Mi). Currently listing Virtual Address Descriptors (VADs) of a process.

Kernel Device Name: `\\Device\\MManager`<br>

List of User-Mode applications:
- vadlist.exe