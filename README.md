# x64-cod4rad

A 64-bit, GPU-capable fork of the `cod4rad.exe` source reconstruction, the lighting compiler from the Call of Duty 4 mod tools (originally recovered from the shipped 2007 binary).

Compared to the original reconstruction it adds:

- an **x64 build**, for maps that run out of the 32-bit address space
- **GPU radiosity tracing** (`-gpu`), using Direct3D 11 compute
- up to **16 worker threads** (the original was limited to 4)
- a crash report that names the function and line of an unhandled exception

The executable is still called `cod4rad.exe`, so it can replace the one in the mod tools.

## Requirements

- Visual Studio 2026 with the C++ toolset for the platform you build:
  **x64** for the 64-bit build, **x86 (32-bit)** for the byte-exact Win32 build
  (Desktop development with C++ → "MSVC ... C++ x64/x86 build tools")
- Premake 5 (bundled as `tools\premake5.exe`)
- For `-gpu`: a Direct3D 11 capable GPU

## Build

```
generate-buildfiles_vs26.bat
```

Then open `build\cod4rad.slnx` in Visual Studio 2026 and build the platform you want, or from a command line:

```
msbuild build\cod4rad.slnx /p:Configuration=Release /p:Platform=x64
msbuild build\cod4rad.slnx /p:Configuration=Release /p:Platform=Win32
```

| Platform | Executable | Notes |
| --- | --- | --- |
| x64 | `bin\x64\cod4rad.exe` | No 32-bit memory limit. Uses SSE2, so it is **not byte-exact** with the original. |
| Win32 | `bin\cod4rad.exe` | Byte-exact with the original (see Notes). Limited to the 32-bit address space. |

## Usage

```
cod4rad -platform pc raw\maps\mp\<mapname>
```

Run it from the [Call of Duty 4 mod tools](https://github.com/promod/CoD4-Mod-Tools) directory.

### GPU radiosity (`-gpu`)

Add `-gpu` to trace the radiosity rays on the GPU. It prints which GPU it uses and, at the end of the
light transport phase, a short timing report. The CPU path stays the default; GPU traces use float math
instead of the original x87 code, so results can differ slightly. If no GPU is available it falls back to the CPU.

Sun and point light shadows, the light grid and model lighting still run on the CPU.

### Threads

`-Threads N` accepts 1 to 16. Without it the CPU path uses up to 4 threads, as the original did, and
`-gpu` uses every core (up to 16), because the CPU side is what keeps the GPU fed.

### Quality and memory

`-SuperSample`, `-Traces` and `-Extra` raise quality, and memory use grows with them (roughly with
`-Traces` × `-SuperSample`²). A map that fails with "Out of memory" in the Win32 build should work in the x64 build.

### Crash reports

If the program crashes, it prints the exception, the function and source line, and a call stack, and
writes the same to `cod4rad_crash.txt` in the current directory. Keep `cod4rad.pdb` next to the exe to get names.

## Notes

The Win32 build uses `/arch:IA32 /fp:precise` on purpose: the original is an x87
build, and SSE2 code generation changes floating-point results and therefore
the output bytes. The x64 build cannot do this, which is why it is not byte-exact.
