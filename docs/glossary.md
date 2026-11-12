# Glossary

Short explanations of terms used in the kohjoh guides. For detailed hardware
background, see Coffeepot's [CTR glossary](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/glossary.md).

| Term | Meaning here |
| --- | --- |
| **ABI** | The rules compiled code uses to pass values and lay out data. The DS game code and 3DS libraries use different rules, so kohjoh keeps them on separate sides of [`kj_bridge.h`](../src/runtime/kj_bridge.h). |
| **Azahar** | A Nintendo 3DS emulator used to boot builds, run input scripts and capture images. It is not a substitute for Old 3DS performance testing. |
| **CIA** | An installable 3DS title package. kohjoh's design describes a future CIA, but the current build guide and verified artifact are for `.3dsx`. |
| **Decompilation** | Recreated C/C++ source corresponding to a game. kohjoh compiles the pinned HGSS decompilation; it is not rebuilding the game from its original commercial source files. |
| **DS** | Nintendo DS, the original system for HeartGold and SoulSilver. “DS-side” in kohjoh refers to the compatibility layer and game code that emulate DS services and graphics state. |
| **ELF** | A linked executable format used as an intermediate/debugging artifact before packaging for the 3DS. |
| **Frame publish** | The point at which the game hands one completed set of graphics state to the renderer. It happens at a defined game frame boundary ([`kj_frame.c`](../src/runtime/kj_frame.c)). |
| **GPU** | Graphics processing unit. The 3DS PICA200 draws kohjoh's converted 2D and 3D work. |
| **HGSS** | HeartGold and SoulSilver. |
| **MSYS2** | A Windows development environment that provides Unix-like shells and tools. Paths and environment variables can differ between MSYS2 tools and native Windows programs. |
| **Nitro / NitroSDK** | Nintendo's DS software libraries and interfaces used by the original game. kohjoh keeps compatibility code for the game while replacing the renderer. |
| **O3DS / Old 3DS** | The original 3DS hardware model, with less CPU/GPU headroom than New 3DS models. A performance claim must be measured on the intended hardware. |
| **PICA200** | The graphics processor in the Nintendo 3DS family. It supports vertex shaders and fixed-function pixel operations; see Coffeepot's [GPU overview](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/hardware/gpu.md). |
| **RomFS** | Read-only content storage commonly embedded in an installed title. kohjoh's path code can look for a game pack there, but full CIA packaging is not implemented. |
| **VBlank / vertical blank** | The short interval between display refreshes. The game uses it as a timing point; kohjoh's frame bridge relates DS VBlanks to 3DS display refreshes. |
| **VRAM** | Video memory. The DS game uses mapped banks for backgrounds, sprites, textures and palettes. kohjoh publishes those mappings for the renderer. |
| **`.3dsx`** | Homebrew executable format launched through a 3DS homebrew loader. The currently documented kohjoh package target. |
