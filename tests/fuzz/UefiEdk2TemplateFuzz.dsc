## @file
#  Host-based AFL++ fuzzing platform. Built like tests/unit (UnitTestFrameworkPkg host build).
##

[Defines]
  PLATFORM_NAME           = UefiEdk2TemplateFuzzPkg
  PLATFORM_GUID           = 38300D3F-D7E1-4A20-90F1-F4D3F16C5A00
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiEdk2TemplateFuzzPkg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

  DEFINE UNIT_TESTING_CODE_COVERAGE_ENABLE = FALSE

!ifndef FUZZ_CC
  DEFINE FUZZ_CC = afl-gcc-fast
!endif
  DEFINE FUZZ_SAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

!if $(TOOL_CHAIN_TAG) != "GCC"
  !error Fuzzing is set up for the GCC toolchain only (afl-gcc-fast)
!endif

[BuildOptions]
  # all modules (EDK2 libs too) are compiled with ASan + UBSan
  GCC:*_*_*_CC_FLAGS = $(FUZZ_SAN_FLAGS)

[BuildOptions.common.EDKII.HOST_APPLICATION]
  # only the fuzz binary: AFL compiler + linking with the sanitizer runtimes
  GCC:*_*_*_CC_PATH     == $(FUZZ_CC)
  GCC:*_*_*_DLINK_PATH  == $(FUZZ_CC)
  GCC:*_*_*_DLINK_FLAGS = $(FUZZ_SAN_FLAGS)

[Components]
  $(PKG_DIR)/$(MODULE_INF) {
    <BuildOptions>
      GCC:*_*_X64_CC_FLAGS = -I$(MODULE_DIR)/include
  }