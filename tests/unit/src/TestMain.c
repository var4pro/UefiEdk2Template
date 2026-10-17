#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include "Greeting.h"

#define UNIT_TEST_APP_NAME "UefiEdk2Template Unit Tests"
#define UNIT_TEST_APP_VERSION "1.0"

STATIC UNIT_TEST_STATUS EFIAPI TestFormats(IN UNIT_TEST_CONTEXT Context) {
    CHAR16 Buf[32];

    UT_ASSERT_NOT_EFI_ERROR(BuildGreeting(Buf, ARRAY_SIZE(Buf), L"World"));
    UT_ASSERT_EQUAL(StrCmp(Buf, L"Hello, World!"), 0);
    return UNIT_TEST_PASSED;
}

STATIC UNIT_TEST_STATUS EFIAPI TestBadParams(IN UNIT_TEST_CONTEXT Context) {
    CHAR16 Buf[32];

    UT_ASSERT_STATUS_EQUAL(BuildGreeting(NULL, 32, L"x"), EFI_INVALID_PARAMETER);
    UT_ASSERT_STATUS_EQUAL(BuildGreeting(Buf, 32, NULL), EFI_INVALID_PARAMETER);
    return UNIT_TEST_PASSED;
}

STATIC UNIT_TEST_STATUS EFIAPI TestTooSmall(IN UNIT_TEST_CONTEXT Context) {
    CHAR16 Buf[4];

    UT_ASSERT_STATUS_EQUAL(BuildGreeting(Buf, ARRAY_SIZE(Buf), L"World"), EFI_BUFFER_TOO_SMALL);
    return UNIT_TEST_PASSED;
}

int main(int argc, char* argv[]) {
    UNIT_TEST_FRAMEWORK_HANDLE Fw = NULL;
    UNIT_TEST_SUITE_HANDLE Suite = NULL;

    if (EFI_ERROR(InitUnitTestFramework(&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION))) return 1;

    if (!EFI_ERROR(CreateUnitTestSuite(&Suite, Fw, "Greeting", "UefiEdk2Template.Greeting", NULL, NULL))) {
        AddTestCase(Suite, "Formats greeting", "Format", TestFormats, NULL, NULL, NULL);
        AddTestCase(Suite, "Rejects bad params", "Params", TestBadParams, NULL, NULL, NULL);
        AddTestCase(Suite, "Rejects small buffer", "Small", TestTooSmall, NULL, NULL, NULL);
        RunAllTestSuites(Fw);
    }

    FreeUnitTestFramework(Fw);
    return 0;
}