# x64-cod4rad

A 64-bit, GPU-accelerated fork of the `cod4rad.exe` source reconstruction, the lighting compiler from the Call of Duty 4 mod tools (originally recovered from the shipped 2007 binary).

> [!WARNING]
> **This project was made entirely with AI.**
> Every change in this fork, the 64-bit port, the GPU tracer, the threading and the build files, was written by an AI model, and the underlying code is a machine-assisted reconstruction of a decompiled binary. It works on the maps it has been tried on, but it has not been reviewed the way real software should be. **Do not treat it as an example of good programming**, and do not copy its patterns into code you care about. Back up your map files before compiling with it, and compare the output against the original tool if the result matters.

## ✨ What this fork adds

- 🚀 **GPU radiosity tracing** (`-gpu`) on any Direct3D 11 GPU
- 🧠 **64-bit build**, so big maps and heavy settings stop running out of memory
- 🧵 **Up to 16 threads** (the original stopped at 4)
- 🩺 **Crash reports** that name the function and line instead of silently closing

The executable is still called `cod4rad.exe`, so it can replace the one in your mod tools.

## 🚀 GPU radiosity

Add `-gpu` and the radiosity rays are traced on your graphics card. The GPU builds the rays itself and only a small job record per lightmap sample goes over the bus, so the CPU is left with the bookkeeping.

```
cod4rad -platform pc -gpu -Extra -SuperSample 8 -Traces 128 raw\maps\mp\<mapname>
```

On one test map (about 5.4 billion rays, RTX 5080, 16 threads) the light transport phase took about 40 seconds. At the end of that phase it prints a short timing report so you can see where the time went.

- ⚡ Float math on the GPU instead of the original x87 code, so results can differ slightly from the CPU path.
- 🔁 If there's no usable GPU, it says so and falls back to the CPU.
- 🧱 Still on the CPU: sun and point light shadows, the light grid and model lighting.
- `-Threads N` accepts 1 to 16. Without it, the CPU path uses up to 4 threads like the original, and `-gpu` uses every core (up to 16), since the CPU side is what keeps the GPU fed.

## Build

Requirements:

- Visual Studio 2026 with the C++ toolset for the platform you want: **x64** for the 64-bit build, **x86** for the byte-exact Win32 build
- Premake 5 (bundled as `tools\premake5.exe`)
- A Direct3D 11 capable GPU, only for `-gpu`

```
generate-buildfiles_vs26.bat
```

Then open `build\cod4rad.slnx` in Visual Studio and build the platform you want, or from a command line:

```
msbuild build\cod4rad.slnx /p:Configuration=Release /p:Platform=x64
msbuild build\cod4rad.slnx /p:Configuration=Release /p:Platform=Win32
```

| Platform | Output | Notes |
| --- | --- | --- |
| x64 | `bin\x64\cod4rad.exe` | No 32-bit memory limit. Uses SSE2, so it is **not byte-exact** with the original. |
| Win32 | `bin\cod4rad.exe` | Byte-exact with the original (see below). Limited to the 32-bit address space. |

## Usage

```
cod4rad -platform pc raw\maps\mp\<mapname>
```

Run it from the [Call of Duty 4 mod tools](https://github.com/promod/CoD4-Mod-Tools) directory.

### Quality and memory

`-SuperSample`, `-Traces` and `-Extra` raise quality, and memory use grows with them (roughly with `-Traces` × `-SuperSample`²). If the Win32 build fails with "Out of memory", try the x64 build.

### Crash reports

If the program crashes, it prints the exception, the function and source line, and a call stack, and writes the same to `cod4rad_crash.txt` in the current directory. Keep `cod4rad.pdb` next to the exe to get names.

## Notes

The Win32 build uses `/arch:IA32 /fp:precise` on purpose: the original is an x87 build, and SSE2 code generation changes floating-point results and therefore the output bytes. The x64 build cannot do this, which is why it is not byte-exact.
