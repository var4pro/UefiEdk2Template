# ===========================================================================
# UEFI EDK2 template - build, run and test helper
#
# Usage (WORKSPACE_DIR_V = folder containing edk2/, with BaseTools built):
#   make build           WORKSPACE_DIR_V=<ws>                   build the driver (TARGET_V=RELEASE|DEBUG)
#   make run             WORKSPACE_DIR_V=<ws> DISK_DIR_V=<dir>  build, copy as EFI/BOOT/BOOTX64.EFI, start QEMU
#   make run-unit-test   WORKSPACE_DIR_V=<ws>                   build and run the host unit tests
#   make clean | clean-unit-test | deep-clean
#   make generate-flags  WORKSPACE_DIR_V=<ws>                   regenerate compile_flags.txt (clangd/clang-tidy)
#   make init            WORKSPACE_DIR_V=<ws>                   first-time setup: git hooks + template remote
#   make print-<VAR>                                            show a variable, e.g. print-BUILD_OUT_V
#   EXTRA_FLAGS_V="..."  extra args for the EDK2 build, e.g. EXTRA_FLAGS_V="-D ANALYZER=TRUE"
#
# Contract with both .dsc files (root and tests/unit):
#   [Components]      $(PKG_DIR)/$(MODULE_INF)  <- both passed by this Makefile via -D
#   OUTPUT_DIRECTORY  literal path, unique per platform (deep-clean deletes it)
# ===========================================================================

SHELL := /bin/bash

.DELETE_ON_ERROR:
# Goals run one at a time, so "make clean build" can't run both at once.
# EDK2's build is parallel on its own (-n 0 = one thread per CPU).
.NOTPARALLEL:
-include $(CURDIR)/local.mk

ROOT_C_FILES_V   := $(shell find src -type f -name "*.c" 2>/dev/null)
ROOT_H_FILES_V   := $(shell find include -type f -name "*.h" 2>/dev/null)
ROOT_SRC_FILES_V := $(ROOT_C_FILES_V) $(ROOT_H_FILES_V)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
# $(call get-define,<file>,<KEY>) -> value of the first "KEY = value" line.
get-define = $(strip $(if $(1),$(shell sed -n -E 's/^[[:space:]]*$(2)[[:space:]]*=[[:space:]]*([^[:space:]]+).*/\1/p' '$(1)' | head -n 1)))

# $(call abs-out-dir,<OUTPUT_DIRECTORY>) -> absolute path.
# OUTPUT_DIRECTORY is relative to the EDK2 WORKSPACE (= edk2/) unless it's absolute.
# Never build a path from an empty OUTPUT_DIRECTORY
#    (otherwise "$(EDK2_PATH_V)/" + rm -rf = the whole edk2 folder)
abs-out-dir = $(if $(strip $(1)),$(if $(filter /%,$(1)),$(1),$(EDK2_PATH_V)/$(1)),$(error [ERROR] Empty OUTPUT_DIRECTORY - refusing to build an output path))

# $(call edk2-build,<dsc relative to PACKAGES_PATH>,<TARGET>,<inf relative to project root>)
# Passes PKG_DIR and MODULE_INF to the DSC, so [Components] is: $(PKG_DIR)/$(MODULE_INF)
# WORKSPACE is pinned so a WORKSPACE exported in your shell can't move Build/.
edk2-build = cd $(EDK2_PATH_V) && \
    export WORKSPACE="$$PWD" && \
    export PACKAGES_PATH="$$PWD:$(abspath $(CURDIR)/..)" && \
    export EDK_TOOLS_PATH="$$PWD/BaseTools" && \
    source edksetup.sh && \
    build -n 0 -a $(ARCH_V) -t $(TOOLCHAIN_V) -p $(1) -b $(2) \
          -D PKG_DIR=$(PKG_DIR_V) -D MODULE_INF=$(3) $(EXTRA_FLAGS_V)

# ---------------------------------------------------------------------------
# Auto-detected project info (from the first .inf / .dsc in the project root)
# ---------------------------------------------------------------------------
# This folder's name, as seen through PACKAGES_PATH (which contains the parent dir)
PKG_DIR_V := $(notdir $(CURDIR))

# sort => deterministic "first" file (wildcard order isn't guaranteed)
INF_FILES_V     := $(sort $(wildcard $(CURDIR)/*.inf))
DSC_FILES_V     := $(sort $(wildcard $(CURDIR)/*.dsc))
ROOT_INF_FILE_V := $(firstword $(INF_FILES_V))
ROOT_DSC_FILE_V := $(firstword $(DSC_FILES_V))

BASE_NAME_V        := $(call get-define,$(ROOT_INF_FILE_V),BASE_NAME)
PLATFORM_NAME_V    := $(call get-define,$(ROOT_DSC_FILE_V),PLATFORM_NAME)
# Must be a literal path in the DSC (no $(...) macros), make can't expand those
OUTPUT_DIRECTORY_V := $(call get-define,$(ROOT_DSC_FILE_V),OUTPUT_DIRECTORY)

override ARCH_V := X64
BOOT_NAME_V     := BOOTX64.EFI
BIOS_V          ?= $(firstword $(wildcard /usr/share/edk2/x64/OVMF_CODE.4m.fd /usr/share/OVMF/OVMF_CODE_4M.fd))

# Passed to edk2-build: .dsc relative to PACKAGES_PATH, .inf relative to this folder
DSC_V             := $(PKG_DIR_V)/$(notdir $(ROOT_DSC_FILE_V))
ROOT_MODULE_INF_V := $(notdir $(ROOT_INF_FILE_V))
TARGET_V          ?= RELEASE
TOOLCHAIN_V       := GCC
EXTRA_FLAGS_V     ?=

# paths
WORKSPACE_DIR_V ?=
DISK_DIR_V      ?=

# $(call expand-path,<path>) -> absolute path with a leading ~ expanded
# (zsh/fish pass VAR=~/x to make literally, and make's abspath doesn't expand ~)
expand-path = $(abspath $(patsubst ~/%,$(HOME)/%,$(patsubst ~,$(HOME),$(strip $(1)))))

ifneq ($(strip $(WORKSPACE_DIR_V)),)
override WORKSPACE_DIR_V := $(call expand-path,$(WORKSPACE_DIR_V))
endif
ifneq ($(strip $(DISK_DIR_V)),)
override DISK_DIR_V := $(call expand-path,$(DISK_DIR_V))
endif
export EDK2_PATH_V := $(WORKSPACE_DIR_V)/edk2

# EDK2 output layout:
#   <OUTPUT_DIRECTORY>/<TARGET>_<TOOLCHAIN>/<ARCH>/                             final binaries (named after BASE_NAME)
#   <OUTPUT_DIRECTORY>/<TARGET>_<TOOLCHAIN>/<ARCH>/<PKG_DIR>/<inf without .inf> module intermediates (named after the .inf file)
BUILD_OUT_V       = $(call abs-out-dir,$(OUTPUT_DIRECTORY_V))/$(TARGET_V)_$(TOOLCHAIN_V)/$(ARCH_V)
ROOT_MODULE_DIR_V = $(BUILD_OUT_V)/$(PKG_DIR_V)/$(basename $(ROOT_MODULE_INF_V))
ROOT_EFI_V        = $(BUILD_OUT_V)/$(BASE_NAME_V).efi


CURRENT_GOALS_V := $(or $(MAKECMDGOALS),all)
# Goals that require WORKSPACE_DIR_V (and a valid .inf/.dsc)
WORKSPACE_GOALS_V := all build copy run clean deep-build deep-clean deep-format-check-all hook-check analyzer \
                     build-unit-test copy-unit-test run-unit-test clean-unit-test
# Goals that strictly require DISK_DIR_V (build and clean excluded)
DISK_GOALS_V := copy run

ifneq ($(filter $(WORKSPACE_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(strip $(WORKSPACE_DIR_V)),)
$(error [ERROR] Variable WORKSPACE_DIR_V isn't set! Set it on invoking make)
endif
ifeq ($(ROOT_INF_FILE_V),)
$(error [ERROR] No .inf file found in $(CURDIR))
endif
ifeq ($(ROOT_DSC_FILE_V),)
$(error [ERROR] No .dsc file found in $(CURDIR))
endif
ifeq ($(BASE_NAME_V),)
$(error [ERROR] BASE_NAME not found in $(ROOT_INF_FILE_V))
endif
ifeq ($(PLATFORM_NAME_V),)
$(error [ERROR] PLATFORM_NAME not found in $(ROOT_DSC_FILE_V))
endif
ifeq ($(OUTPUT_DIRECTORY_V),)
$(error [ERROR] OUTPUT_DIRECTORY not found in $(ROOT_DSC_FILE_V))
endif
ifneq ($(word 2,$(INF_FILES_V)),)
$(warning [WARN] Multiple .inf files in root, using $(notdir $(ROOT_INF_FILE_V)))
endif
ifneq ($(word 2,$(DSC_FILES_V)),)
$(warning [WARN] Multiple .dsc files in root, using $(notdir $(ROOT_DSC_FILE_V)))
endif
endif

ifneq ($(filter $(DISK_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(strip $(DISK_DIR_V)),)
$(error [ERROR] Variable DISK_DIR_V isn't set! Set it on invoking make)
endif
endif

.PHONY: all build copy run clean deep-build deep-clean generate-flags format-do tidy deep-format-check-all hook-check analyzer init \
        build-unit-test copy-unit-test run-unit-test clean-unit-test

all: build
#build
build:
	@$(call edk2-build,$(DSC_V),$(TARGET_V),$(ROOT_MODULE_INF_V))

copy: build
	@if [ ! -f "$(ROOT_EFI_V)" ]; then \
        echo "[ERROR] $(ROOT_EFI_V) not found"; exit 1; \
    fi
	mkdir -p $(DISK_DIR_V)/EFI/BOOT
	cp -f "$(ROOT_EFI_V)" $(TARGET_EFI_V)


TARGET_EFI_V := $(DISK_DIR_V)/EFI/BOOT/$(BOOT_NAME_V)
run: copy
	@[ -f "$(BIOS_V)" ] || { echo "[ERROR] OVMF firmware not found ('$(BIOS_V)'). Install edk2-ovmf or set BIOS_V"; exit 1; }
	qemu-system-x86_64 \
        -drive if=pflash,format=raw,readonly=on,file=$(BIOS_V) \
        -drive format=raw,file=fat:rw:$(DISK_DIR_V) \
        -net none

#clean
clean:
	rm -rf $(ROOT_MODULE_DIR_V) $(ROOT_EFI_V) $(BUILD_OUT_V)/$(BASE_NAME_V).debug
	rm -f compile_flags.txt


#flags
generate-flags: 
	@rm -f compile_flags.txt
	@$(MAKE) compile_flags.txt

compile_flags.txt: compile_flags.txt.in
	@if [ -z "$(strip $(WORKSPACE_DIR_V))" ]; then \
        echo "[ERROR] WORKSPACE_DIR_V is not set! Please set it before running."; \
        exit 1; \
    fi
	@echo "Generating compile_flags.txt..."
	@envsubst '$$EDK2_PATH_V' < $< > $@

#tidy
tidy: compile_flags.txt 
	$(MAKE) -C tools/clang-tidy-uefi build
	clang-tidy --warnings-as-errors='*' --load=tools/clang-tidy-uefi/build/libUefiTidyModule.so $(ROOT_C_FILES_V)


#unit host-based tests:
UT_DIR_V              := tests/unit
UT_INF_FILES_V        := $(sort $(wildcard $(CURDIR)/$(UT_DIR_V)/*.inf))
UT_DSC_FILES_V        := $(sort $(wildcard $(CURDIR)/$(UT_DIR_V)/*.dsc))
UT_INF_FILE_V         := $(firstword $(UT_INF_FILES_V))
UT_DSC_FILE_V         := $(firstword $(UT_DSC_FILES_V))
UT_BASE_NAME_V        := $(call get-define,$(UT_INF_FILE_V),BASE_NAME)
UT_PLATFORM_NAME_V    := $(call get-define,$(UT_DSC_FILE_V),PLATFORM_NAME)
UT_OUTPUT_DIRECTORY_V := $(call get-define,$(UT_DSC_FILE_V),OUTPUT_DIRECTORY)
UT_TARGET_V           ?= NOOPT

# Passed to edk2-build (same idea as DSC_V / ROOT_MODULE_INF_V)
UT_DSC_V        := $(PKG_DIR_V)/$(UT_DIR_V)/$(notdir $(UT_DSC_FILE_V))
UT_MODULE_INF_V := $(UT_DIR_V)/$(notdir $(UT_INF_FILE_V))

UT_BUILD_OUT_V  = $(call abs-out-dir,$(UT_OUTPUT_DIRECTORY_V))/$(UT_TARGET_V)_$(TOOLCHAIN_V)/$(ARCH_V)
UT_MODULE_DIR_V = $(UT_BUILD_OUT_V)/$(PKG_DIR_V)/$(basename $(UT_MODULE_INF_V))
UT_BIN_V        = $(UT_BUILD_OUT_V)/$(UT_BASE_NAME_V)

UNIT_TEST_C_FILES_V   := $(shell find $(UT_DIR_V)/src -type f -name "*.c" 2>/dev/null)
UNIT_TEST_H_FILES_V   := $(shell find $(UT_DIR_V)/include -type f -name "*.h" 2>/dev/null)
UNIT_TEST_SRC_FILES_V := $(UNIT_TEST_C_FILES_V) $(UNIT_TEST_H_FILES_V)

UT_GOALS_V := build-unit-test copy-unit-test run-unit-test clean-unit-test deep-build deep-clean

ifneq ($(filter $(UT_GOALS_V),$(CURRENT_GOALS_V)),)
ifeq ($(UT_INF_FILE_V),)
$(error [ERROR] No .inf file found in $(UT_DIR_V))
endif
ifeq ($(UT_DSC_FILE_V),)
$(error [ERROR] No .dsc file found in $(UT_DIR_V))
endif
ifeq ($(UT_BASE_NAME_V),)
$(error [ERROR] BASE_NAME not found in $(UT_INF_FILE_V))
endif
ifeq ($(UT_PLATFORM_NAME_V),)
$(error [ERROR] PLATFORM_NAME not found in $(UT_DSC_FILE_V))
endif
ifeq ($(UT_OUTPUT_DIRECTORY_V),)
$(error [ERROR] OUTPUT_DIRECTORY not found in $(UT_DSC_FILE_V))
endif
ifneq ($(word 2,$(UT_INF_FILES_V)),)
$(warning [WARN] Multiple .inf files in $(UT_DIR_V), using $(notdir $(UT_INF_FILE_V)))
endif
ifneq ($(word 2,$(UT_DSC_FILES_V)),)
$(warning [WARN] Multiple .dsc files in $(UT_DIR_V), using $(notdir $(UT_DSC_FILE_V)))
endif
endif

build-unit-test:
	@$(call edk2-build,$(UT_DSC_V),$(UT_TARGET_V),$(UT_MODULE_INF_V))

copy-unit-test: build-unit-test
	@if [ ! -f "$(UT_BIN_V)" ]; then \
        echo "[ERROR] $(UT_BIN_V) not found"; exit 1; \
    fi
	cp -f "$(UT_BIN_V)" ./$(UT_BASE_NAME_V)

run-unit-test: copy-unit-test
	./$(UT_BASE_NAME_V)

clean-unit-test:
	rm -rf $(UT_MODULE_DIR_V) $(UT_BIN_V)
	rm -f ./$(UT_BASE_NAME_V)







#for normal and unit testing
#format
FORMAT_FILES_V = $(ROOT_SRC_FILES_V) $(UNIT_TEST_SRC_FILES_V)
format-do:
	@echo "Formatting code with clang-format..."
	@if [ -n "$(FORMAT_FILES_V)" ]; then \
        clang-format -i $(FORMAT_FILES_V); \
        echo "Formatting done!"; \
    else \
        echo "No source files found to format."; \
    fi
	@$(MAKE) -C tools/clang-tidy-uefi format-do

deep-build: build build-unit-test
	$(MAKE) -C tools/clang-tidy-uefi build
deep-clean: clean clean-unit-test
	rm -rf $(call abs-out-dir,$(OUTPUT_DIRECTORY_V)) $(call abs-out-dir,$(UT_OUTPUT_DIRECTORY_V))
	$(MAKE) -C tools/clang-tidy-uefi clean
        
#analyzer: #TODO MAKE IT WORKS LATER
#Checking everything
deep-format-check-all: format-do hook-check #manually invoke this

hook-check: compile_flags.txt deep-build tidy #auto invoking
	$(MAKE) -C tools/clang-tidy-uefi hook-check WORKSPACE_DIR_V=$(WORKSPACE_DIR_V)


#tools
print-%:
	@echo '$* = $($*)  (origin: $(origin $*))'

TEMPLATE_URL_V := https://github.com/var4pro/UefiEdk2Template.git
init: generate-flags
	git config core.hooksPath .githooks
	@git remote add template $(TEMPLATE_URL_V) 2>/dev/null || git remote set-url template $(TEMPLATE_URL_V)
	git remote set-url --push template DISABLED