[Defines]
  PLATFORM_NAME           = UefiEdk2TemplateTestPkg
  PLATFORM_GUID           = 87594535-BC16-4BD0-9EE2-3E4747872C58
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiEdk2TemplateTestPkg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Components]
  $(PKG_DIR)/$(MODULE_INF) {
    <BuildOptions>
      GCC:*_*_X64_CC_FLAGS = -I$(MODULE_DIR)/include
  }