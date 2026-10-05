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

## Usage

```
cod4rad -platform pc raw\maps\mp\<mapname>
```

Run it from the [Call of Duty 4 mod tools](https://github.com/promod/CoD4-Mod-Tools) directory.

## Notes

The build uses `/arch:IA32 /fp:precise` on purpose: the original is an x87
build, and SSE2 code generation changes floating-point results and therefore
the output bytes.
