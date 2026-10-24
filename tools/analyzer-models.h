// ===========================================================================
// EDK2 library contracts for GCC, force-included into every file of the
// module by the .dsc (-include). Used by "make analyzer" and by plain -Wall.
// GCC only knows libc (malloc, memcpy, strlen...); these attributes describe
// the EDK2 equivalents. Every one was checked against the EDK2 doc comment AND
// the implementation (edk2/MdePkg/Library): a wrong one = false positives, so
// check the library code before adding (the docs are sometimes wrong).
// X64 only for now (MDE_CPU_X64), GCC >= 15 (nonnull_if_nonzero).
//
// These are EDK2 MdePkg LIBRARY functions, not UEFI spec ones:
//   MemoryAllocationLib, BaseMemoryLib, BaseLib, PrintLib, DevicePathLib, UefiLib
// Most of them (AllocateZeroPool, CompareMem, StrLen, ...) aren't in the spec at all.
// AllocatePool/FreePool/AllocatePages/FreePages/CopyMem/SetMem share their names
// with spec boot services, but the spec ones are gBS->AllocatePool (Type, Size,
// &Buffer) etc.: function pointers returning EFI_STATUS, a different API.
// The spec (UEFI 2.11, 7.2 Memory Allocation Services) still backs two rules,
// since the library forwards to those services:
//   FreePool: Buffer "must have been allocated by AllocatePool()" and an invalid
//   Buffer gives EFI_INVALID_PARAMETER -> malloc(FreePool) + nonnull below.
// Direct gBS->AllocatePool()/FreePool() calls and anything returned through an
// OUT argument (GetVariable2, EfiLocateProtocolBuffer, ...) are NOT covered:
// attributes can't describe a buffer handed back through a pointer argument.
//
// "make analyzer" analyzes our objects WITHOUT the EDK2 library code (with the
// whole module the analyzer runs out of budget inside BasePrintLib & co, see the
// Makefile), so every library call is opaque. What the attributes add:
//
// malloc(FreePool, 1): the result may be NULL and must be released with FreePool
//   -Wanalyzer-possible-null-dereference, -Wanalyzer-malloc-leak,
//   -Wanalyzer-double-free, -Wanalyzer-use-after-free,
//   -Wanalyzer-mismatching-deallocation (FreePool on AllocatePages memory)
//   Also on DevicePathLib/UefiLib functions that return a new pool buffer
//   (DuplicateDevicePath, ConvertDevicePathToText, CatSPrint, ...).
//   Only the malloc(Dealloc, Index) form is used: it changes diagnostics only.
//   Plain "malloc" would also let the optimizer assume the new block holds no
//   pointers, which is false for AllocateCopyPool().
//   ReallocatePool*() is NOT modeled: it frees OldBuffer only on success, GCC
//   would treat it as always freed -> false double-free on the correct
//   "if (New == NULL) FreePool (Old);" path. Pointers passed to it are simply
//   no longer tracked.
//   Linking UefiMemoryAllocationLib into the analysis (ANALYZER_LIBS_V) cancels
//   these models: the analyzer then walks the real code into gBS->FreePool().
//   Leaks are reported only with "make analyzer ANALYZER_LEAKS_V=1": GCC reports
//   a false leak when only a field's address leaves the function
//   (InsertTailList (&List, &Node->Link), InstallProtocolInterface (..., &Private->Protocol)).
//   A pointer passed to any other opaque call counts as handed off, so
//   "S = CatSPrint (S, ...)" or a buffer only passed to Print() isn't reported either.
//
// nonnull / nonnull_if_nonzero (Ptr, Length): only where the code really
//   ASSERTs (or dereferences) on NULL. NOT where EDK2 handles NULL as an error:
//   StrCpyS & co return RETURN_INVALID_PARAMETER, StrDecimalToUintn & co return
//   0 (their docs say ASSERT, the code doesn't). CopyMem/SetMem have no NULL
//   check at all (address 0 is valid memory in firmware).
//   FreePool(NULL) is a bug in UEFI (unlike free(NULL)): the core returns
//   EFI_INVALID_PARAMETER and the library ASSERTs.
//   -Wanalyzer-possible-null-argument, -Wnonnull
//   nonnull normally also lets the optimizer drop NULL checks; the .dsc builds
//   with -fno-delete-null-pointer-checks, which turns that off (generated code
//   is identical with and without this header). Keep that flag.
//
// access (read_only/write_only/read_write, Ptr, Size): how many elements of
//   *Ptr the function may touch (DestMax of StrCpyS, Length of CopyMem, ...)
//   -Wstringop-overflow (compile time), -Wanalyzer-out-of-bounds
//   Size counts elements of the pointer type, so it's left out where EDK2
//   passes bytes for a CHAR16/UINT16+ buffer (UnicodeSPrint, CalculateSum16, ...).
//
// null_terminated_string_arg (N): CHAR8 strings that must end with '\0'
//   -Wanalyzer-out-of-bounds (over-read). CHAR16 strings can't be described
//   (the attribute looks for a 0 byte).
// ===========================================================================
#pragma once

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 15

    #include <Base.h>

    #if defined(MDE_CPU_X64)

        #include <Library/MemoryAllocationLib.h>
        #include <Library/BaseMemoryLib.h>
        #include <Library/BaseLib.h>
        #include <Library/PrintLib.h>

        // Re-declare an EDK2 function with its exact type plus extra GCC attributes.
        // Doesn't compile if the function isn't declared (e.g. IA32-only), so nothing is invented.
        #define ANALYZER_CONTRACT_V(Func, ...) extern __typeof__(Func) Func __attribute__((__VA_ARGS__))

// --- MemoryAllocationLib ---------------------------------------------------
ANALYZER_CONTRACT_V(AllocatePool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateRuntimePool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateReservedPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateZeroPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateRuntimeZeroPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateReservedZeroPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateCopyPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateRuntimeCopyPool, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AllocateReservedCopyPool, malloc(FreePool, 1));

ANALYZER_CONTRACT_V(AllocatePages, malloc(FreePages, 1));
ANALYZER_CONTRACT_V(AllocateRuntimePages, malloc(FreePages, 1));
ANALYZER_CONTRACT_V(AllocateReservedPages, malloc(FreePages, 1));

ANALYZER_CONTRACT_V(AllocateAlignedPages, malloc(FreeAlignedPages, 1));
ANALYZER_CONTRACT_V(AllocateAlignedRuntimePages, malloc(FreeAlignedPages, 1));
ANALYZER_CONTRACT_V(AllocateAlignedReservedPages, malloc(FreeAlignedPages, 1));

ANALYZER_CONTRACT_V(FreePool, nonnull(1));
ANALYZER_CONTRACT_V(FreePages, nonnull(1));
ANALYZER_CONTRACT_V(FreeAlignedPages, nonnull(1));

// --- BaseMemoryLib ---------------------------------------------------------
ANALYZER_CONTRACT_V(CopyMem, access(write_only, 1, 3), access(read_only, 2, 3));
ANALYZER_CONTRACT_V(SetMem, access(write_only, 1, 2));
ANALYZER_CONTRACT_V(ZeroMem, access(write_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(SetMem16, access(write_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(SetMem32, access(write_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(SetMem64, access(write_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(SetMemN, access(write_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(CompareMem, access(read_only, 1, 3), access(read_only, 2, 3), nonnull_if_nonzero(1, 3),
                    nonnull_if_nonzero(2, 3));
ANALYZER_CONTRACT_V(ScanMem8, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(ScanMem16, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(ScanMem32, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(ScanMem64, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(ScanMemN, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(ScanGuid, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(IsZeroBuffer, access(read_only, 1, 2), nonnull_if_nonzero(1, 2));
ANALYZER_CONTRACT_V(CopyGuid, nonnull(1, 2));
ANALYZER_CONTRACT_V(CompareGuid, nonnull(1, 2));
ANALYZER_CONTRACT_V(IsZeroGuid, nonnull(1));

// --- BaseLib: strings ------------------------------------------------------
// CHAR16
ANALYZER_CONTRACT_V(StrLen, nonnull(1));
ANALYZER_CONTRACT_V(StrSize, nonnull(1));
ANALYZER_CONTRACT_V(StrCmp, nonnull(1, 2));
ANALYZER_CONTRACT_V(StrnCmp, nonnull_if_nonzero(1, 3), nonnull_if_nonzero(2, 3));
ANALYZER_CONTRACT_V(StrStr, nonnull(1, 2));
// CHAR8
ANALYZER_CONTRACT_V(AsciiStrLen, nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(AsciiStrSize, nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(AsciiStrCmp, nonnull(1, 2), null_terminated_string_arg(1), null_terminated_string_arg(2));
ANALYZER_CONTRACT_V(AsciiStriCmp, nonnull(1, 2), null_terminated_string_arg(1), null_terminated_string_arg(2));
ANALYZER_CONTRACT_V(AsciiStrnCmp, nonnull_if_nonzero(1, 3), nonnull_if_nonzero(2, 3));
ANALYZER_CONTRACT_V(AsciiStrStr, nonnull(1, 2), null_terminated_string_arg(1), null_terminated_string_arg(2));
// safe strings: NULL is a handled error (no nonnull), DestMax = elements of Destination
ANALYZER_CONTRACT_V(StrCpyS, access(write_only, 1, 2));
ANALYZER_CONTRACT_V(StrnCpyS, access(write_only, 1, 2));
ANALYZER_CONTRACT_V(StrCatS, access(read_write, 1, 2));
ANALYZER_CONTRACT_V(StrnCatS, access(read_write, 1, 2));
ANALYZER_CONTRACT_V(AsciiStrCpyS, access(write_only, 1, 2), null_terminated_string_arg(3));
ANALYZER_CONTRACT_V(AsciiStrnCpyS, access(write_only, 1, 2));
ANALYZER_CONTRACT_V(AsciiStrCatS, access(read_write, 1, 2), null_terminated_string_arg(3));
ANALYZER_CONTRACT_V(AsciiStrnCatS, access(read_write, 1, 2));
ANALYZER_CONTRACT_V(UnicodeStrToAsciiStrS, access(write_only, 2, 3));
ANALYZER_CONTRACT_V(UnicodeStrnToAsciiStrS, access(write_only, 3, 4));
ANALYZER_CONTRACT_V(AsciiStrToUnicodeStrS, access(write_only, 2, 3), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(AsciiStrnToUnicodeStrS, access(write_only, 3, 4));
ANALYZER_CONTRACT_V(StrHexToBytes, access(write_only, 3, 4));
ANALYZER_CONTRACT_V(AsciiStrHexToBytes, access(write_only, 3, 4));

// --- BaseLib: linked lists -------------------------------------------------
ANALYZER_CONTRACT_V(InitializeListHead, nonnull(1));
ANALYZER_CONTRACT_V(InsertHeadList, nonnull(1, 2));
ANALYZER_CONTRACT_V(InsertTailList, nonnull(1, 2));
ANALYZER_CONTRACT_V(GetFirstNode, nonnull(1));
ANALYZER_CONTRACT_V(GetNextNode, nonnull(1, 2));
ANALYZER_CONTRACT_V(GetPreviousNode, nonnull(1, 2));
ANALYZER_CONTRACT_V(IsListEmpty, nonnull(1));
ANALYZER_CONTRACT_V(IsNull, nonnull(1, 2));
ANALYZER_CONTRACT_V(IsNodeAtEnd, nonnull(1, 2));
ANALYZER_CONTRACT_V(IsNodeInList, nonnull(1, 2));
ANALYZER_CONTRACT_V(SwapListEntries, nonnull(1, 2));
ANALYZER_CONTRACT_V(RemoveEntryList, nonnull(1));

// --- BaseLib: checksums (Length in bytes: access only for byte/VOID buffers) ---
ANALYZER_CONTRACT_V(CalculateSum8, nonnull(1), access(read_only, 1, 2));
ANALYZER_CONTRACT_V(CalculateCheckSum8, nonnull(1), access(read_only, 1, 2));
ANALYZER_CONTRACT_V(CalculateSum16, nonnull(1));
ANALYZER_CONTRACT_V(CalculateCheckSum16, nonnull(1));
ANALYZER_CONTRACT_V(CalculateSum32, nonnull(1));
ANALYZER_CONTRACT_V(CalculateCheckSum32, nonnull(1));
ANALYZER_CONTRACT_V(CalculateSum64, nonnull(1));
ANALYZER_CONTRACT_V(CalculateCheckSum64, nonnull(1));
ANALYZER_CONTRACT_V(CalculateCrc32, nonnull(1), access(read_only, 1, 2));
ANALYZER_CONTRACT_V(CalculateCrc16CcittF, nonnull(1), access(read_only, 1, 2));
ANALYZER_CONTRACT_V(CalculateCrc16Ansi, access(read_only, 1, 2));
ANALYZER_CONTRACT_V(CalculateCrc32c, access(read_only, 1, 2));

// --- BaseLib: jumps --------------------------------------------------------
ANALYZER_CONTRACT_V(SetJump, nonnull(1));
ANALYZER_CONTRACT_V(LongJump, nonnull(1));

// --- PrintLib (BufferSize in bytes: access only for CHAR8 output) ------------
ANALYZER_CONTRACT_V(SPrintLength, nonnull(1));
ANALYZER_CONTRACT_V(SPrintLengthAsciiFormat, nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(AsciiSPrint, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2),
                    null_terminated_string_arg(3));
ANALYZER_CONTRACT_V(AsciiVSPrint, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2),
                    null_terminated_string_arg(3));
ANALYZER_CONTRACT_V(AsciiBSPrint, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2),
                    null_terminated_string_arg(3));
ANALYZER_CONTRACT_V(AsciiSPrintUnicodeFormat, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2));
ANALYZER_CONTRACT_V(AsciiVSPrintUnicodeFormat, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2));
ANALYZER_CONTRACT_V(AsciiBSPrintUnicodeFormat, access(write_only, 1, 2), nonnull_if_nonzero(1, 2), nonnull_if_nonzero(3, 2));
ANALYZER_CONTRACT_V(AsciiValueToStringS, access(write_only, 1, 2), nonnull(1));

        // --- UEFI-only libraries (need Uefi.h types: DXE/UEFI modules) ---------------
        #if defined(EFI_SYSTEM_TABLE_SIGNATURE)

            #include <Library/DevicePathLib.h>
            #include <Library/UefiLib.h>

// DevicePathLib: new pool buffers
ANALYZER_CONTRACT_V(DuplicateDevicePath, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AppendDevicePath, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AppendDevicePathNode, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(AppendDevicePathInstance, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(GetNextDevicePathInstance, malloc(FreePool, 1), nonnull(2));
ANALYZER_CONTRACT_V(CreateDeviceNode, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(FileDevicePath, malloc(FreePool, 1), nonnull(2));
ANALYZER_CONTRACT_V(ConvertDevicePathToText, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(ConvertDeviceNodeToText, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(ConvertTextToDevicePath, malloc(FreePool, 1));
ANALYZER_CONTRACT_V(ConvertTextToDeviceNode, malloc(FreePool, 1));
// DevicePathLib: node accessors
ANALYZER_CONTRACT_V(DevicePathType, nonnull(1));
ANALYZER_CONTRACT_V(DevicePathSubType, nonnull(1));
ANALYZER_CONTRACT_V(DevicePathNodeLength, nonnull(1));
ANALYZER_CONTRACT_V(NextDevicePathNode, nonnull(1));
ANALYZER_CONTRACT_V(IsDevicePathEndType, nonnull(1));
ANALYZER_CONTRACT_V(IsDevicePathEnd, nonnull(1));
ANALYZER_CONTRACT_V(IsDevicePathEndInstance, nonnull(1));
ANALYZER_CONTRACT_V(SetDevicePathNodeLength, nonnull(1));
ANALYZER_CONTRACT_V(SetDevicePathEndNode, nonnull(1));

// UefiLib: new pool buffers (CatSPrint doesn't free String: "S = CatSPrint (S, ...)" leaks)
ANALYZER_CONTRACT_V(GetBestLanguage, malloc(FreePool, 1), nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(CatSPrint, malloc(FreePool, 1), nonnull(2));
ANALYZER_CONTRACT_V(CatVSPrint, malloc(FreePool, 1), nonnull(2));
// UefiLib: output
ANALYZER_CONTRACT_V(Print, nonnull(1));
ANALYZER_CONTRACT_V(ErrorPrint, nonnull(1));
ANALYZER_CONTRACT_V(AsciiPrint, nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(AsciiErrorPrint, nonnull(1), null_terminated_string_arg(1));
ANALYZER_CONTRACT_V(PrintXY, nonnull(5));
ANALYZER_CONTRACT_V(AsciiPrintXY, nonnull(5), null_terminated_string_arg(5));
// UefiLib: variables, tables, events, locks
ANALYZER_CONTRACT_V(GetVariable2, nonnull(1, 2, 3));
ANALYZER_CONTRACT_V(GetVariable3, nonnull(1, 2, 3));
ANALYZER_CONTRACT_V(GetEfiGlobalVariable2, nonnull(1, 2));
ANALYZER_CONTRACT_V(EfiGetSystemConfigurationTable, nonnull(1, 2));
ANALYZER_CONTRACT_V(EfiCreateProtocolNotifyEvent, nonnull(1, 3, 5));
ANALYZER_CONTRACT_V(EfiNamedEventListen, nonnull(1, 3));
ANALYZER_CONTRACT_V(EfiNamedEventSignal, nonnull(1));
ANALYZER_CONTRACT_V(EfiCreateEventLegacyBoot, nonnull(1));
ANALYZER_CONTRACT_V(EfiCreateEventLegacyBootEx, nonnull(4));
ANALYZER_CONTRACT_V(EfiCreateEventReadyToBoot, nonnull(1));
ANALYZER_CONTRACT_V(EfiCreateEventReadyToBootEx, nonnull(4));
ANALYZER_CONTRACT_V(EfiInitializeLock, nonnull(1));
ANALYZER_CONTRACT_V(EfiAcquireLock, nonnull(1));
ANALYZER_CONTRACT_V(EfiAcquireLockOrFail, nonnull(1));
ANALYZER_CONTRACT_V(EfiReleaseLock, nonnull(1));
ANALYZER_CONTRACT_V(EfiTestManagedDevice, nonnull(3));
ANALYZER_CONTRACT_V(EfiTestChildHandle, nonnull(3));
ANALYZER_CONTRACT_V(EfiInitializeFwVolDevicepathNode, nonnull(1, 2));
ANALYZER_CONTRACT_V(EfiGetNameGuidFromFwVolDevicePathNode, nonnull(1));
// UefiLib: driver model
ANALYZER_CONTRACT_V(EfiLibInstallDriverBinding, nonnull(3));
ANALYZER_CONTRACT_V(EfiLibInstallAllDriverProtocols, nonnull(3));
ANALYZER_CONTRACT_V(EfiLibInstallDriverBindingComponentName2, nonnull(3));
ANALYZER_CONTRACT_V(EfiLibInstallAllDriverProtocols2, nonnull(3));
ANALYZER_CONTRACT_V(EfiLibUninstallDriverBinding, nonnull(1));
ANALYZER_CONTRACT_V(EfiLibUninstallAllDriverProtocols, nonnull(1));
ANALYZER_CONTRACT_V(EfiLibUninstallDriverBindingComponentName2, nonnull(1));
ANALYZER_CONTRACT_V(EfiLibUninstallAllDriverProtocols2, nonnull(1));

        #endif // EFI_SYSTEM_TABLE_SIGNATURE

        // force-included everywhere: don't leak the helper macro into user code
        #undef ANALYZER_CONTRACT_V

    #endif // MDE_CPU_X64
#endif     // GCC >= 15
