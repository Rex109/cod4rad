[![License](https://img.shields.io/badge/license-MIT-blue)](https://creativecommons.org/licenses/by-nc/4.0/)
[![Personal Discord](https://img.shields.io/discord/953653773962739793?color=%237289DA&label=Personal%20Discord&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/QDYk75vBBk)
[![ManyAsset](https://img.shields.io/discord/585171589750849538?color=%23FF8711&label=ManyAsset&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/v2TWkeR)

# x64-CoD4Rad
<img width="1820" height="396" alt="logo" src="https://github.com/user-attachments/assets/10edfe8f-ade1-4058-8f39-e3ea0f991042"/>

*<p align="center"><sub>Let there be light!</sub></p>*
<br>
A 64-bit, GPU-accelerated fork of the `cod4rad.exe` source reconstruction, the lighting compiler from the Call of Duty 4 mod tools (originally recovered from the shipped 2007 binary).
<br>

> [!WARNING]
> **This project was made entirely with AI.**
> Every change in this fork, the 64-bit port, the GPU tracer, the threading and the build files, was written by an AI model, and the underlying code is a machine-assisted reconstruction of a decompiled binary. It works on the maps it has been tried on, but it has not been reviewed the way real software should be. **Do not treat it as an example of good programming**, and do not copy its patterns into code you care about.

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
- 🧊 The light grid's sky traces (the bulk of its work) run on the GPU too, and so does the colour search of its quantization (the "Improving quantization" step used by `-Extra`).
- 🧱 Still on the CPU: sun, point light and emissive brush shadows, and the rest of the light grid and model lighting.
- `-Threads N` accepts 1 to 16. Without it, the CPU path uses up to 4 threads like the original, and `-gpu` uses every core (up to 16), since the CPU side is what keeps the GPU fed.

## 💡 Emissive brushes/Area lights
<img width="100%" alt="iw3mp_fPtZ12n4mL" src="https://github.com/user-attachments/assets/9ea9251c-7a50-4600-936b-349c3df2f322" />

Make a brush part of a brush entity (for example `script_brushmodel`) and give it these keys in Radiant. When cod4rad runs, the brush's visible faces give off light, with no material changes.

| Key | Meaning |
| --- | --- |
| `_emit_color` | `r g b`, scaled so the brightest component is 1 (same as a light's `_color`) |
| `_emit_intensity` | Brightness, default `1`. It is what a very large face gives a surface right next to it |
| `_emit_radius` | How far the light reaches, default `512` (it fades smoothly to nothing there) |
| `_emit_samples` | Points sampled on the brush for every lit point, default `16`, up to `256`. More is smoother and slower |

- The faces are real **area lights** for everything around them: light from each face falls off with distance squared and with the angle, so big panels give soft shadows.
- Light goes **outwards only**, along each face's normal. Nothing is lit behind a face, the brush does not light its own surfaces, and the faces of a convex brush don't light each other.
- Surfaces and models (through the light grid) are both lit. The light is baked, so it does not move with the entity at runtime.
- Surfaces very close to a face can look slightly noisy with few samples; raise `_emit_samples` on that brush.
- cod4rad prints `emissive brush *N: ...` for each one it finds, so you can check it was picked up.

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
