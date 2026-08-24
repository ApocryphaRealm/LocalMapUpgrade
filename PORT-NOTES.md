# Port notes — INI-only settings → SKSE Menu Framework

**Version 3.1.2.** This is a fork of
[alexsylex/LocalMapUpgrade](https://github.com/alexsylex/LocalMapUpgrade) 3.1.0 that adds an
in-game settings page driven by
[SKSE Menu Framework 3](https://github.com/QTR-Modding/SKSE-Menu-Framework-3), so the local
map's rendering and marker behavior can be configured while the game is running instead of
only through `LocalMapUpgrade.ini` at startup.

The upstream `README.md` and git history are untouched.

**Versioning note**: this build's compiled-in version tracks upstream (3.1.0) with the third
part bumped for this fork's own changes, rather than starting fresh at this project's usual
1.0.0. That is a deliberate exception: Dragon's Eye Minimap depends on Local Map Upgrade and
checks its declared plugin version at load through SKSE's messaging interface
(`skse->GetPluginInfo("LocalMapUpgrade")->version >= 0x03010000`, in its own
`source/MessageListeners.cpp`) before it will run. The very first build of this fork shipped
as its own 1.0.0, which is a real regression under that numeric check even though the fork is
a strict superset of the upstream feature/API surface Dragon's Eye Minimap actually depends
on - it failed at runtime with "Local Map Upgrade 3.1.0 or newer required" the moment both
mods were installed together. A plugin's own declared version is a cross-mod compatibility
signal other mods can query, not purely an internal package label, so for a mod that is itself
a dependency of another mod in this project, tracking upstream's numbering (bumped for local
changes) is correct and the "own versioning starts at 1.0.0" default does not apply.

## What changed from upstream

| File | Change |
| --- | --- |
| `include/SKSEMenuFramework.h` | Vendored, unmodified, from SKSE Menu Framework 3.13. Reaches the framework through `GetProcAddress`, so nothing has to be linked. |
| `include/UI.h`, `source/UI.cpp` | New. The settings page, the registration, and the live-apply entry point. |
| `include/Settings.h` | Added `Save()`, `RestoreDefaults()`, `Reload()` and `GetIniPath()` declarations; the settings themselves are unchanged from upstream. |
| `source/Settings.cpp` | Captures the compiled-in values as defaults before reading the INI, and can write or re-read every setting. Reads go through a null-safe helper rather than dereferencing the collection directly, and registration goes through a checked helper - both hardening moves ported over from Dragon's Eye Minimap's own port, after that one crashed on startup once from exactly this class of bug (see that repo's PORT-NOTES.md). This codebase didn't have a live instance of that bug - every setting's declared type already matched its name prefix - but the same fragile pattern (`INISettingCollection::GetSetting<T>` dereferencing a possibly-null pointer) is used here too, so the same protection was applied preemptively rather than waiting to hit it. |
| `include/ShaderManager.h`, `source/ShaderManager.cpp` | Added `SetFogOfWar(bool)`, a version of the existing `ToggleFogOfWarLocalMapShader()` that sets a specific value instead of only flipping the current one - needed so a settings-menu checkbox can apply its own state directly. Behavior of the existing toggle function is unchanged (it now just calls the new setter with the flipped value). **3.1.2**: `SetPixelShaderProperties()` now also writes the shape/style it was called with back into the singleton's own `shape`/`style` members - see "The color/minimap bug fixed in 3.1.2" below. |
| `source/MessageListeners.cpp` | Calls `UI::Register()` on `kPostPostLoad`. |
| `CMakeLists.txt` | Version tracks upstream (3.1.0), bumped for this fork's own changes - see the versioning note above. Auto-deploy copy is now conditional on the target game folder existing (`EXISTS` check added) - the original always ran the copy step and failed the build on a machine without that exact Steam install path. |
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

### The color/minimap bug fixed in 3.1.2

Confirmed in game by the author: toggling Color changed the paused Local Map screen immediately but
never reached Dragon's Eye Minimap, which kept showing whatever color setting was active at
game load. Root cause: `ShaderManager::SetPixelShaderProperties(shape, style)` always applied
the requested shape/style to the actual GPU shader pointer (`localMapPixelShader->shader`),
but never wrote them back into the `ShaderManager` singleton's own `shape`/`style` members -
only `GetPixelShaderProperties()` reads those, and it was therefore always returning this
instance's construction-time value, frozen at whatever `localMapColor` was when the plugin
first loaded.

Dragon's Eye Minimap calls that Get/Set pair every frame in its own `WorldRendering.cpp`: it
calls `GetPixelShaderProperties()` to snapshot the *current* shape and style, calls `Set()`
with its own round shape and that snapshotted style to draw the minimap, then calls `Set()`
again with the original shape and that same snapshotted style to restore the local map's own
shader afterward. Because the snapshot never moved off the frozen startup value, that restore
step was silently reverting the shared shader back to the original color setting on every
single frame - which is also why the minimap itself never showed anything else, since it was
drawn with that same stale snapshot too.

This bug already existed upstream - the Get/Set hook and Dragon's Eye Minimap's use of it both
predate this fork - but it was latent, because nothing in the original mod ever called `Set()`
a second time after startup (there was no live toggle). Adding one is what exposed it. Fixed
by having `SetPixelShaderProperties()` also update `singleton->shape`/`singleton->style`
whenever it runs (guarded against the one call made from the constructor itself, before
`singleton` is assigned). Fog of war was never affected by this, since `isFogOfWarEnabled` is
tracked as a single always-current variable rather than a per-instance snapshot.

Local Map Upgrade's actor-visibility toggles (enemy/hostile/guard/dead/teammate/neutral, and
immersive mode) are unrelated to this bug and were never expected to affect the minimap:
`ExtraMarkersManager::AddExtraMarkers` hooks specifically into the paused Local Map screen's
own marker-list rebuild (`RE::LocalMapMenu`). Dragon's Eye Minimap has no actor-marker
rendering of its own at all - it only ever mirrors the local map's rendering style (color, fog
of war, shape), never its marker icons. Making the minimap show actor markers would be new
functionality built into Dragon's Eye Minimap itself, not a fix to this port.

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
