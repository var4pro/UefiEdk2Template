#ifndef GREETING_H_
#define GREETING_H_

#include <Uefi.h>

EFI_STATUS
BuildGreeting(OUT CHAR16* Buffer, IN UINTN BufferChars, IN CONST CHAR16* Name);

#endif