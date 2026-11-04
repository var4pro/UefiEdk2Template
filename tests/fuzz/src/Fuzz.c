#include <Uefi.h>
#include <unistd.h> // read(), used by the AFL macros

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Fuzz.h"

// Function under test
EFI_STATUS GetName(IN CONST UINT8* Buffer, IN UINTN Size, OUT CHAR8 Name[NAME_MAX_SIZE]) {
    UINTN Length;

    if (Size < 1) return EFI_BAD_BUFFER_SIZE;

    Length = Buffer[0];
    if ((Length >= NAME_MAX_SIZE) || (Length > Size - 1)) return EFI_BAD_BUFFER_SIZE;

    CopyMem(Name, &Buffer[1], Length);
    Name[Length] = '\0';
    return EFI_SUCCESS;
}

__AFL_FUZZ_INIT();

// afl-fuzz: persistent loop over shared memory. By hand: ./<bin> < file (one run)
int main(VOID) {
    UINT8* AflBuffer;
    UINT8* Input;
    UINTN Size;
    CHAR8 Name[NAME_MAX_SIZE];

    __AFL_INIT();
    AflBuffer = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        Size = __AFL_FUZZ_TESTCASE_LEN;

        // Exact-size copy, so ASan catches reads past the end of the input
        Input = AllocateCopyPool(Size, AflBuffer);
        if (Input == NULL) continue;

        (VOID) GetName(Input, Size, Name);
        FreePool(Input);
    }

    return 0;
}