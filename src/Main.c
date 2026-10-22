#include "Nothing.h"

#include <PiDxe.h>                            // <-- Use PiDxe.h for DXE Drivers!
#include <Library/UefiLib.h>                  // For Print
#include <Library/UefiBootServicesTableLib.h> // For gST and gBS
#include <Library/UefiDriverEntryPoint.h>     // For DriverEntryPoint
#include <Library/BaseLib.h>                  // For CpuDeadLoop
#include <Library/DebugLib.h>

EFI_STATUS EFIAPI DriverEntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    (void)gST->ConOut->ClearScreen(gST->ConOut);

    Print(L"=========================================\n");
    Print(L"  Hello World from UEFI DXE Driver!      \n");
    Print(L"=========================================\n");
    DEBUG((DEBUG_INFO, "Hello, UART!\n"));

    // UINT8* P = NULL;
    //*P = 0; // expect -Wanalyzer-null-dereference
    CpuDeadLoop();
    return EFI_SUCCESS;
}