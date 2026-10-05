# cod4rad

A source reconstruction of `cod4rad.exe`, the lighting compiler from the Call of Duty 4 mod tools, recovered from the shipped 2007 binary.

## Requirements

- Visual Studio 2026 with the **x86 (32-bit) C++ toolset** installed
  (Desktop development with C++ → "MSVC ... C++ x64/x86 build tools")
- Premake 5 (bundled as `tools\premake5.exe`)

## Build

```
generate-buildfiles_vs26.bat
```

Then open `build\cod4rad.slnx` in Visual Studio 2026 and build the **Win32** platform, or from a command line:

```
msbuild build\cod4rad.slnx /p:Configuration=Release /p:Platform=Win32
```

The executable is written to `bin\cod4rad.exe`.

### 64-bit build

Build the **x64** platform instead (`/p:Platform=x64`, or pick x64 in Visual Studio) for maps that run out of
the 32-bit address space ("Out of memory" errors at high `-SuperSample` / `-Traces`). The executable is
written to `bind\cod4rad.exe`. It always uses SSE2, so it is **not byte-exact** with the original; use the
Win32 build when you need identical output.

## Usage

```
cod4rad -platform pc raw\maps\mp\<mapname>
```

Run it from the [Call of Duty 4 mod tools](https://github.com/promod/CoD4-Mod-Tools) directory.

### GPU radiosity (`-gpu`)

Add `-gpu` to trace the radiosity rays on the GPU (Direct3D 11 compute, any DirectX 11 GPU).
The CPU path stays the default and is the byte-exact one; GPU traces use float math instead of
the original x87 code, so results can differ slightly. If no GPU is available it falls back to the CPU.
Sun and point light shadows, the light grid and model lighting still run on the CPU.

## Notes

The build uses `/arch:IA32 /fp:precise` on purpose: the original is an x87
build, and SSE2 code generation changes floating-point results and therefore
the output bytes.
