/*+================================================================================================
Module Name: dirutil.h
Author     : Paul L. (@am0nsec)
Origin     : https://github.com/am0nsec/wkpe/
Copyright  : This project has been released under the GNU Public License v3 license.

Abstract:
Windows Directory utility code.
Used to change directory to the "PDB" folder and get the correct symbol server search path.
================================================================================================+*/

#ifndef __DIA_DIRUTIL_H_GUARD__
#define __DIA_DIRUTIL_H_GUARD__

#include <Windows.h>

/// <summary>
/// Utility function to get the proper symbol server address to download PDB.
/// </summary>
NTSTATUS STDMETHODCALLTYPE
_Must_inspect_result_
GetSymSrvSearchPath(
	_Out_ PWCHAR* SymSearchPath
);

#endif // !__DIA_DIRUTIL_H_GUARD__
