#pragma once

#include <Uefi.h>

#define NAME_MAX_SIZE 16

// Input: [Length:1][Name:Length]. Copies the name into Name (NUL terminated).
EFI_STATUS
GetName(IN CONST UINT8* Buffer, IN UINTN Size, OUT CHAR8 Name[NAME_MAX_SIZE]);