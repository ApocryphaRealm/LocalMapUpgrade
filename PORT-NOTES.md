# Port notes — INI-only settings → SKSE Menu Framework

**Version 1.0.0.** This is a fork of
[alexsylex/LocalMapUpgrade](https://github.com/alexsylex/LocalMapUpgrade) 3.1.0 that adds an
in-game settings page driven by
[SKSE Menu Framework 3](https://github.com/QTR-Modding/SKSE-Menu-Framework-3), so the local
map's rendering and marker behavior can be configured while the game is running instead of
only through `LocalMapUpgrade.ini` at startup.

The upstream `README.md` and git history are untouched. This project's own versioning starts
at 1.0.0 (the standing default for this project), independent of upstream's 3.1.0.

## What changed from upstream

| File | Change |
| --- | --- |
| `include/SKSEMenuFramework.h` | Vendored, unmodified, from SKSE Menu Framework 3.13. Reaches the framework through `GetProcAddress`, so nothing has to be linked. |
| `include/UI.h`, `source/UI.cpp` | New. The settings page, the registration, and the live-apply entry point. |
| `include/Settings.h` | Added `Save()`, `RestoreDefaults()`, `Reload()` and `GetIniPath()` declarations; the settings themselves are unchanged from upstream. |
| `source/Settings.cpp` | Captures the compiled-in values as defaults before reading the INI, and can write or re-read every setting. Reads go through a null-safe helper rather than dereferencing the collection directly, and registration goes through a checked helper - both hardening moves ported over from Dragon's Eye Minimap's own port, after that one crashed on startup once from exactly this class of bug (see that repo's PORT-NOTES.md). This codebase didn't have a live instance of that bug - every setting's declared type already matched its name prefix - but the same fragile pattern (`INISettingCollection::GetSetting<T>` dereferencing a possibly-null pointer) is used here too, so the same protection was applied preemptively rather than waiting to hit it. |
| `include/ShaderManager.h`, `source/ShaderManager.cpp` | Added `SetFogOfWar(bool)`, a version of the existing `ToggleFogOfWarLocalMapShader()` that sets a specific value instead of only flipping the current one - needed so a settings-menu checkbox can apply its own state directly. Behavior of the existing toggle function is unchanged (it now just calls the new setter with the flipped value). |
| `source/MessageListeners.cpp` | Calls `UI::Register()` on `kPostPostLoad`. |
| `CMakeLists.txt` | Version bumped to this project's own 1.0.0. Auto-deploy copy is now conditional on the target game folder existing (`EXISTS` check added) - the original always ran the copy step and failed the build on a machine without that exact Steam install path. |
| `cmake/ports/commonlibsse-ng/portfile.cmake` | Fetches CommonLibVR over git instead of a GitHub tarball, same as Dragon's Eye Minimap's fork - the pinned SHA512 for the same commit had already rotted (GitHub re-compresses generated tarballs over time) by the time this fork was set up. |

### Why the port file changed

Same root cause and same fix as Dragon's Eye Minimap's fork: the upstream port pinned a
SHA512 of GitHub's generated `.tar.gz` for a specific `CommonLibVR` commit. That hash no
longer matched by the time this build was configured. Fetching the same pinned commit over
git instead keeps the pin (the commit id) while letting git verify the content, which cannot
rot the same way a compressed-archive hash can.

## What's live and what needs a restart

Every setting applies live except nothing - this is a fully live-apply port. Color and fog of
war go through `ShaderManager::SetPixelShaderProperties`/`SetFogOfWar`, which just select
between shader variants that are already compiled at load (both the color and black-and-white
variants, and both the fog-of-war and no-fog-of-war variants, are built upfront) - so toggling
either is instant, no shader recompile needed. Pan speed and every actor-visibility flag
(including immersive mode) are read directly from `settings::mapmenu::*` at the point of use
every time - a keypress, a marker-filter pass - rather than being cached anywhere, so a menu
edit takes effect the next time that code runs, no explicit "apply" call needed for those at
all.

## Building

Same as Dragon's Eye Minimap - Visual Studio (Desktop development with C++) and vcpkg,
neither pinned to a particular install location:

```
set VCPKG_ROOT=C:\path\to\vcpkg
configure.bat
build.bat
```

The DLL lands in `build/relwithdebinfo-se-only/LocalMapUpgrade.dll`. The configured preset is
SE/AE only; use `build-relwithdebinfo-all` for a build that also loads in Skyrim VR.

## Licence

Local Map Upgrade is by alexsylex, MIT licensed. This fork keeps the original licence.
