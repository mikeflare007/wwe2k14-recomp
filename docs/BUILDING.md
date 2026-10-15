# Building WWE 2K14 Recomp from source

This guide builds the Windows x64 executable (cross-compiled from Linux). The Linux
executable route from WWE '13 Recomp has not been tested for 2K14 yet. Nothing here
distributes the original game or ReXGlue source. Supply your own WWE 2K14 files and `tools/setup.sh` fetches and patches the ReXGlue SDK.

## Supported build hosts and requirements

The verified route is **64-bit Ubuntu 24.04** with LLVM 20. The Windows target
is cross-compiled from Linux with Clang's MSVC ABI, `lld-link`, and Microsoft's
Windows SDK/MSVC CRT files supplied by `xwin`.

Minimums and versions used for this build:

- x86-64 Ubuntu/Linux; Ubuntu 24.04 is the tested host.
- CMake 3.25 or newer; tested with 3.28.3.
- Ninja; tested with 1.13.0 (Ninja Multi-Config support is required).
- LLVM/Clang 20, including `clang`, `clang++`, `lld`, `lld-link`, `llvm-ar`,
  `llvm-ranlib`, `llvm-rc`, and `llvm-mt`; tested with 20.1.2.
- Git with recursive submodule support; tested with 2.43.0.
- Python 3.11+ for the helper scripts; tested with 3.12.3.
- For Windows cross-builds: `xwin` 0.10.0 (or a compatible newer version) and
  its downloaded/splatted x86-64 Windows SDK + MSVC CRT.
- Recommended: 32 GB RAM and at least 10 GB free disk space for SDK and game
  build products. Reserve 30–120 minutes for first-time dependency/codegen/build
  work; hardware, network speed, and build parallelism affect both estimates.
  `BUILD_JOBS=8` is the default maximum; lower it on smaller machines.

Install the Linux toolchain and libraries (Ubuntu package names):

```bash
sudo apt-get update
sudo apt-get install -y wget gnupg lsb-release
wget -O /tmp/llvm.sh https://apt.llvm.org/llvm.sh
sudo bash /tmp/llvm.sh 20
sudo apt-get install -y clang-20 lld-20
sudo apt-get install -y cmake ninja-build build-essential git curl unzip autoconf \
  python3.12-venv libgtk-3-dev libx11-xcb-dev wine
```

The clean-build host was Ubuntu 24.04.4 with `build-essential` 12.10ubuntu1,
`libgtk-3-dev` 3.24.41, `libx11-dev` 1.8.7, `libxkbcommon-dev` 1.6.0,
`python3.12-venv` 3.12.3, and Wine 9.0. Package versions may advance with Ubuntu
updates; the CMake/Ninja/LLVM/Python minimums above are the supported constraints.

Install Vulkan SDK from LunarG's Ubuntu 24.04 (`noble`) package repository
(or follow [LunarG's current installation instructions](https://vulkan.lunarg.com/sdk/home)):

```bash
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc \
  | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc >/dev/null
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list \
  http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
sudo apt-get update
sudo apt-get install -y vulkan-sdk
```

ReXGlue's Linux CI installs `vulkan-sdk` as well as the packages above. Set
LLVM 20 first in `PATH` for all following SDK/build commands:

```bash
export PATH=/usr/lib/llvm-20/bin:$PATH
```

For the Windows cross-target, install Rust/Cargo if needed, then install and
populate `xwin` outside the repository:

```bash
if ! command -v cargo >/dev/null 2>&1; then
  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
  source "$HOME/.cargo/env"
fi
cargo install xwin --version 0.10.0
xwin --accept-license --arch x86_64 --cache-dir "$HOME/.cache/xwin" download
xwin --accept-license --arch x86_64 --cache-dir "$HOME/.cache/xwin" unpack
xwin --accept-license --arch x86_64 --cache-dir "$HOME/.cache/xwin" splat \
  --output "$HOME/xwin"
```

The build expects `"$HOME/xwin/sdk/include/um/windows.h"`. If the SDK is in a
different location, set `XWIN_DIR` to the splat root for the Windows build.

## ReXGlue SDK (fetched and patched by `tools/setup.sh`)

This project runs on the ReXGlue recompilation SDK. The release was built with the public fork
[`HollywoodAkeem/rexglue-sdk-yukes`](https://github.com/HollywoodAkeem/rexglue-sdk-yukes) at commit
`987593c0fc6612850a2e1b247b2b03aa5281e663` plus this project's SDK changes, which are kept in
`rexglue-patches/` (one `git am` patch per change). The SDK goes **next to** this repository at
`../tools/rexglue`; all build scripts use that layout.

```bash
mkdir wwe2k14 && cd wwe2k14
git clone <this repository URL> wwe2k14-recomp
cd wwe2k14-recomp
nice -n 10 env BUILD_JOBS=8 ./tools/setup.sh
```

On the first run `tools/setup.sh` clones the fork into `../tools/rexglue`, checks out the pinned commit, applies every
patch in `rexglue-patches/`, fetches the SDK's own submodules, then configures the SDK once into
`../tools/rexglue/out/build/linux-amd64-clang20` (LLVM 20, `-march=x86-64-v3`, Vulkan, shared FFmpeg) and installs it
to `../tools/rexglue/out/install/linux-amd64`. The codegen program is
`../tools/rexglue/out/linux-amd64/RelWithDebInfo/rexglue`. Later runs reuse the existing checkout.

## Provide your own game files and generate `generated/`

This project's function metadata (`config/functions.toml`) is for the WWE 2K14 executable with title update
**2.0.1.0** (media ID `0D94D1D2`, title ID `545408B2`). Codegen needs two files from your own copy. The hashes are
for identifying the right files; the scripts do not enforce them.

| File | Role | Size | SHA-256 |
|---|---|---:|---|
| `default.xex` | WWE 2K14 executable (from your disc) | 24,657,920 bytes | `544ef8b505aebcece3459865a4e27e1a7cfad568406514f27e1115b378d3a8fb` |
| `default.xexp` | title update 2.0.1.0 patch (inside `tu00000002_00000000`) | 2,041,856 bytes | `3ab6cd4cb5f353077847da6165848d01f7684f48e6bc5767e6aad73ec622c361` |

Get `default.xexp` out of your title update package with `tools/extract-title-update.py` (it also gives you the
other update files the game reads at run time):

```bash
python3 tools/extract-title-update.py /path/to/tu00000002_00000000 out/update
install -D -m 0644 "/path/to/your/WWE 2K14/default.xex" game/default.xex
install -D -m 0644 out/update/default.xexp game/default.xexp
```

Generate the recompiled C++ (about 4 to 10 minutes) and check it:

```bash
nice -n 10 ./tools/codegen.sh
python3 tools/check-generated.py
```

`wwe2k14_config.toml` is the codegen entry point. It reads `game/default.xex` (ReXGlue applies the `default.xexp`
next to it), writes to the ignored `generated/` folder, and includes `config/functions.toml` and `config/hooks.toml`.
`generated/` is derived from the game's code: never commit or share it.

`tools/check-generated.py` must pass before you build. It rejects the three codegen problems that caused real bugs in
this port: function splits that turn branches into `return`s, gotos across functions, and split register
save/restore helpers.

### Changing `config/functions.toml`

Add an entry only for a function the analyzer missed (the game logs
`[missing-function] indirect call to 0x...`). Check that the address is a real function start (it follows a
`blr`/`bctr`/`b`, and no branch or `loc_` label reaches into it), give it an `end`, rerun codegen, and run
`tools/check-generated.py`. An entry placed inside an existing function splits it, and that kind of mistake is what
froze the legal screen.

## Build Windows x64 from Linux

After preparing `xwin` above, run the cross-build script. It builds/installs the Windows SDK before building the game; Vulkan is
enabled and D3D12 disabled for this project's cross-target.

```bash
nice -n 10 env BUILD_JOBS=8 XWIN_DIR="$HOME/xwin" \
  LLVM_BIN=/usr/lib/llvm-20/bin ./tools/build-windows.sh
```

Output: `out/build/win-amd64-cross/RelWithDebInfo/wwe2k14.exe` plus `avcodec-58.dll` and `avutil-56.dll`.
A full game build takes 25 to 45 minutes. Changes to `src/` only relink, which takes about a minute.

**A native Windows/MSVC or clang-cl build is not currently supported or
verified.** The supported Windows route is the Linux-hosted Clang/xwin cross
build above. No game launch is necessary to produce the binary.

## Troubleshooting

- **`git am` fails in `tools/setup.sh`:** `../tools/rexglue` must be a fresh clone at the pinned commit. Delete
  `../tools/rexglue` and run `tools/setup.sh` again.
- **Missing SDK submodules:** run `git -C ../tools/rexglue submodule update --init --recursive` and rerun
  `tools/setup.sh`.
- **Wrong compiler selected:** check
  `/usr/lib/llvm-20/bin/clang --version`. `tools/setup.sh` and
  `tools/build-linux.sh` use this toolchain by default; the Windows build script
  uses it unless `LLVM_BIN` is set. The SDK configure is cached; if the SDK
  build directory was configured with another compiler, remove only
  `../tools/rexglue/out/build/linux-amd64-clang20` and rerun `tools/setup.sh`.
- **Codegen reports a missing XEXP:** ensure both files are named exactly
  `game/default.xex` and `game/default.xexp` and that the XEXP is TU 2.0.1.0.
- **Missing generated sources:** run `tools/codegen.sh` before building.
  `generated/` is intentionally ignored and is never shipped by Git.
- **Windows SDK not found:** rerun the `xwin` download/unpack/splat commands and
  set `XWIN_DIR` to the splat directory containing `sdk/include/um/windows.h`.
- **Out of memory:** lower `BUILD_JOBS` (for example `BUILD_JOBS=4`) on the
  `nice`/`env` build commands; the default cap is eight.

## Running

Copy `wwe2k14.exe` and the two DLLs next to `launcher-windows/play-wwe2k14.bat` and `settings.txt`, put the
extracted update files in `update/`, and follow the README. Do not share the built executable: it contains
recompiled game code.
