#include "Greeting.h"
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>

EFI_STATUS
BuildGreeting(OUT CHAR16* Buffer, IN UINTN BufferChars, IN CONST CHAR16* Name) {
    if ((Buffer == NULL) || (Name == NULL) || (BufferChars == 0)) return EFI_INVALID_PARAMETER;

    // "Hello, " (7) + Name + "!" (1) + NUL (1)
    if (StrLen(Name) + 9 > BufferChars) return EFI_BUFFER_TOO_SMALL;

    UnicodeSPrint(Buffer, BufferChars * sizeof(CHAR16), L"Hello, %s!", Name);
    return EFI_SUCCESS;
}