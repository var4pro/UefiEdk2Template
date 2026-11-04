[Defines]
  PLATFORM_NAME           = UefiEdk2TemplateTestPkg
  PLATFORM_GUID           = 87594535-BC16-4BD0-9EE2-3E4747872C58
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiEdk2TemplateTestPkg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!ifdef UT_NO_SAN
  DEFINE UT_SAN_FLAGS = -DUT_NO_SAN=1
!else
  DEFINE UT_SAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all
!endif

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[BuildOptions]
  # all modules (EDK2 libs too) are compiled with ASan + UBSan
  GCC:*_*_*_CC_FLAGS = $(UT_SAN_FLAGS)

[BuildOptions.common.EDKII.HOST_APPLICATION]
  # link with the sanitizer runtimes
  GCC:*_*_*_DLINK_FLAGS = $(UT_SAN_FLAGS)

[Components]
  $(PKG_DIR)/$(MODULE_INF) {
    <BuildOptions>
      GCC:*_*_X64_CC_FLAGS = -I$(MODULE_DIR)/include
  }