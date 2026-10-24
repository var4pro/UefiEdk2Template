# UefiEdk2Template
Template for out-of-tree EDK2 projects (X64 DXE drivers and apps) built with GCC.
One `make` wraps the EDK2 `build` command, QEMU, host unit tests, GCC's static analyzer
and formatting. Your project lives in its own folder, outside the edk2 tree.

## Requirements

- Linux, `make`, `bash`, `git`
- **GCC 16** (the driver is built with `-std=c23` and GCC 15+ flags), `nasm`, `python3`, `uuid-dev`
- An EDK2 checkout with BaseTools built (CI uses `edk2-stable202608`)
- `qemu-system-x86_64` + OVMF (`edk2-ovmf` on Arch, `ovmf` on Debian/Ubuntu) for `make run`
- `gettext` (`envsubst`) for `make generate-flags`; `clang-format` for formatting

Setting up EDK2 once:

```bash
mkdir -p ~/uefi_workspace && cd ~/uefi_workspace
git clone -b edk2-stable202608 https://github.com/tianocore/edk2.git
cd edk2 && git submodule update --init --recursive && make -C BaseTools -j"$(nproc)"
```

`WORKSPACE_DIR_V` is the folder **containing** `edk2/` (here `~/uefi_workspace`).
The project folder can be anywhere; the Makefile adds its parent to `PACKAGES_PATH`.

## Quick start

```bash
git clone --recurse-submodules https://github.com/var4pro/UefiEdk2Template.git MyDriver
cd MyDriver
cp local.mk.example local.mk          # set WORKSPACE_DIR_V / DISK_DIR_V once, no need to pass them every time
make init TEMPLATE_NEW_NAME_V=MyDriver
make build
make run
```

`make init` (run once per new project):
- enables the git pre-commit hook (`.githooks/`)
- adds this repo as the `template` remote, to pull template updates later
- generates `compile_flags.txt` for clangd
- renames the project: `UefiEdk2Template` → your name in the `.inf` / `.dsc` file names, their content
  and the Dockerfile, with fresh `FILE_GUID` / `PLATFORM_GUID`. The name must be a C identifier.
  Without `TEMPLATE_NEW_NAME_V` nothing is renamed.

The folder name doesn't matter to the build, so rename it as you like.

## Layout

```
├── UefiEdk2Template.inf        module: sources, libraries, entry point (DXE_DRIVER)
├── UefiEdk2TemplatePkg.dsc     platform: library mapping, compiler flags, OUTPUT_DIRECTORY
├── src/                        driver sources
├── include/                    driver headers (on the include path)
├── tests/unit/                 host unit tests: own .inf/.dsc, src/, include/
├── tools/
