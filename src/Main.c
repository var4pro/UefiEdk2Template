#include "Nothing.h"

#include <Uefi.h>
#include <Base.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

EFI_STATUS EFIAPI DriverEntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    (void)gST->ConOut->ClearScreen(gST->ConOut);

    Print(L"=========================================\n");
    Print(L"  Hello World from UEFI DXE Driver!      \n");
    Print(L"=========================================\n");

    CpuDeadLoop();
    return EFI_SUCCESS;
}