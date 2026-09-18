# LocalMapUpgrade - changelog

Rule 61: this mod's own history, kept beside the code it describes.

> **The entries below this line were RECONSTRUCTED from `version-ledger.json` on
> 2026-08-27, not written at the time of the change.** They carry only what the ledger
> recorded - the status and the evidence - so they are thinner than a real entry and may
> be missing changes the ledger never captured. Treat them as a starting point rather
> than a record. Everything from the next version onward is written as it happens.

Each version carries its **version-ledger status**: **working** (observed in game),
**untested** (built, not confirmed), **failed** (built but broken; the number was
reclaimed), **scratch** (a hypothesis-test build that never held a real number).

<!-- VERSIONING-RULES -->
> **Versioning rules (CLAUDE.md rules 6 and 48 - identical for mods and documents):**
> * `X.Y.Z`. A change increments the THIRD number. At `.9` the MINOR rolls: `1.0.9 -> 1.1.0`;
>   `1.0.10` never exists.
> * The next number is **LAST WORKING + 1**. A failed, scratch or untested test build does NOT
>   consume its number - the next attempt at the same step REUSES it.
> * Numbers are never typed. ONE tool, `.MD\scripts\version-gate.ps1`, holds every version rule
>   and is a GATE that fails: `bump` issues the next number and writes every location, `record`
>   proves it working (evidence + the binary's hash), `gate` refuses packaging or finalizing
>   anything it did not issue. Documents go through `docs-pipeline.ps1 -Action bump` and the rules
>   through `rules-version.ps1 -Action bump`, both of which take their arithmetic from that same
>   tool. A number typed by hand is wrong until the tool agrees.

## 1.3.1 - 2026-09-18 - untested

### Changed
- The Address Library pre-check runs before anything else at load: a missing Address Library file for the game version
  gets a message naming the file and the plugin loads inert, instead of CommonLibSSE-NG's bare failure line
  (oproso's report on the Perfected Wheeler page, 2026-09-18: a guard placed after SKSE::Init never ran). No other change.

## 1.3.0 - 2026-09-16 - untested

### Changed
- Relicensed to GPL-3.0-or-later. The mod links CommonLibSSE-NG and builds against the SKSE Menu Framework header, both GPL-3.0, so the MIT licence it shipped with was never available to it. alexsylex's original MIT notice is preserved in THIRD_PARTY_NOTICES.md, as his licence requires. No code changed.

## 1.2.8 - 2026-09-07 - working

### Added
- The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL (Interface/Translations/LocalMapUpgrade_<language>.txt) and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback. The framework is looked up by its sort-first name first; localmapupgrade.status gained op=strings.

## 1.2.7 - 2026-09-01 - untested

### Added
- A BORDER STYLE selector on the settings page, shown when the map border is on:
  - **Skyrim** (the default): the Nordic knotwork frame drawn round the local map. It places the
    real MO2 "Skyrim" (Trosski) frame art - the same nine-slice the Apocrypha Menu Framework's
    Skyrim theme uses - shipped beside the plugin as three textures and assembled from eight
    clips: four corners at the art's own size, mirrored so one tile serves all four, and four
    edges stretched along their runs. If the art cannot be placed the plain line is drawn
    instead, so the border is never simply missing.
  - **Untarnished**: the plain single line in Untarnished UI's off-white, which is what this mod
    drew before. Available, not the default.
  Standing rule behind it (the author, 2026-09-01): every UI element we draw defaults to the
  Skyrim knotwork, with a UI-replacer style offered alongside and the whole element switchable
  off for anyone on vanilla UI.
- `localmapupgrade.status` gains op=borderstyle, so the styles can be switched live for testing.

### Changed
- THE MAP BORDER NOW SHIPS OFF (design decision, 2026-09-01). It was on by default from 1.2.5.
  The game already draws its own frame round the local map and most UI replacers draw one too,
  so turning ours on unasked stacks a second frame on top of somebody else's - which is a worse
  first impression than no frame at all. It is now a thing you switch ON because you want it,
  typically because your replacer removed the vanilla frame. Compiled default, shipped INI, the
  settings page's own help text and the Nexus page all say the same thing.
- The shipped INI gained `uLocalMapBorderStyle`, which existed in code from earlier in this
  version but had no key in the file.

### Note on verification
The selector and the fallback are confirmed in game. Whether Scaleform loads the frame art at
runtime is NOT yet confirmed: the border only draws while the map is open, and the headless gate
cannot reach the local map (spliced keys do not land in a menu's own context). Left untested in
the ledger until it is seen.

## 1.2.6 - 2026-08-31 - working

### Fixed
- The settings menu now registers with Apocrypha Menu Framework (AMF), the parallel framework
  these mods ship with, by resolving AMF's real module name with stock SKSE Menu Framework as
  the fallback. It previously resolved only the stock name, which AMF's VFS alias deliberately
  refuses at load - so on an AMF stack the settings page never registered ("SKSE Menu Framework
  does not export AddSectionItem" in the log) and the menu was silently absent. Same fix as
  Dragon's Eye Minimap 1.5.8.
- Settings reloaded after a save could come back as the values from game start rather than the ones just written. Save() writes the INI with plain file I/O, but the reload went back through INISettingCollection::ReadFromFile, which uses the Win32 profile APIs that PrivateProfileRedirector hooks and caches - so the reload was served a cache, and with the Redirector configured to flush that cache back to disk it could also overwrite saved settings between sessions. Settings are now parsed straight from the INI with plain file I/O and preferred over the collection, and the INI is never handed to the profile API in either direction. Same fix as Dragon's Eye Minimap 1.5.7, where the bug was first diagnosed end to end.

## 1.2.5 - 2026-08-27 - working

### Changed
- git tag v1.2.5 pushed; finalized package + zip in 10. finalized mods

### Known
- The main package bundles an Untarnished UI SMF theme inside itself (SMF Theme - Untarnished UI 1.0.0\SKSE\plugins\SKSEMenuFrameworkThemes\UntarnishedUI-SMFTheme.json). That is base material carrying an optional theme's identity, the same problem found in the banners. Flagged for a decision, not changed - it alters an already-published package.
- IconDisplayExtensionArt.swf is MARKER artwork, not frame artwork - its symbols are TeammateMarker, NeutralMarker, hIconClip. The Untarnished UI variant differs from the main one by exactly 27 bytes across the whole 41,029-byte decompressed body: 9 colour records, #FFFFFF->#F5F2E9 (x4), #969696->#908E89 (x4), #B4B4B4->#ADABA4 (x1). It is a pure palette shift and needs no Flash toolchain to reproduce - the SWF is CWS/zlib and unpacks with zlib alone.
- The local map border is drawn FROM C++, not from the SWF. ExtraMarkersManager::DrawMapBorder creates an LMUMapBorder clip at runtime and strokes it with lineStyle plus four moveTo/lineTo calls. It is positioned centre-relative - centreX = (left+right)*0.5 + (right-left)*kPlateCentreOffsetX - which is what makes it resolution-independent; an edge-relative anchor breaks across resolutions. The plate centre was measured empirically from two screenshots at (396.6,223.8) and (396.4,223.6) against a stage centre of (400,225). Searching the SWF symbol table for a frame finds nothing and is NOT evidence there is no frame.

## 1.2.4 - 2026-08-27 - working

### Changed
- PROGRESS.md in-game test 2026-08-26 - toggle recolour and up/down nudge confirmed live

## 1.2.3 - 2026-08-27 - untested

### Changed
- local package only - no tag; changelog says explicitly it was not a public release (the swallow, reverted at 1.2.5)

## 1.2.2 - 2026-08-27 - untested

### Changed
- local package only - no tag

## 1.2.1 - 2026-08-27 - working

### Changed
- git tag v1.2.1 pushed

## 1.2.0 - 2026-08-27 - working

### Changed
- git tag v1.2.0 pushed

## 1.1.9 - 2026-08-27 - working

### Changed
- git tag v1.1.9 pushed

## 1.1.8 - 2026-08-27 - working

### Changed
- git tag v1.1.8 pushed

## 1.1.7 - 2026-08-27 - working

### Changed
- git tag v1.1.7 pushed

## 1.1.6 - 2026-08-27 - working

### Changed
- git tag v1.1.6 pushed

## 1.1.5 - 2026-08-27 - working

### Changed
- git tag v1.1.5 pushed

## 1.1.4 - 2026-08-27 - working

### Changed
- git tag v1.1.4 pushed

## 1.1.3 - 2026-08-27 - working

### Changed
- git tag v1.1.3 pushed

## 1.1.2 - 2026-08-27 - working

### Changed
- git tag v1.1.2 pushed

## 1.1.1 - 2026-08-27 - working

### Changed
- git tag v1.1.1 pushed

## 1.1.0 - 2026-08-27 - working

### Changed
- git tag v1.1.0 pushed

## 1.0.9 - 2026-08-27 - working

### Changed
- published on Nexus 189625 (file_id 795087, MAIN, 2026-08-25); git tag v1.0.9 - LOCKED, do not renumber

## 1.0.8 - 2026-08-27 - working

### Changed
- git tag v1.0.8 pushed

## 1.0.7 - 2026-08-27 - working

### Changed
- git tag v1.0.7 pushed

## 1.0.6 - 2026-08-27 - working

### Changed
- git tag v1.0.6 pushed

## 1.0.5 - 2026-08-27 - working

### Changed
- git tag v1.0.5 pushed (source reads 3.1.5)

## 1.0.4 - 2026-08-27 - working

### Changed
- git tag v1.0.4 pushed (source reads 3.1.4)

## 1.0.3 - 2026-08-27 - working

### Changed
- git tag v1.0.3 pushed (source reads 3.1.3)

## 1.0.2 - 2026-08-27 - working

### Changed
- git tag v1.0.2 pushed (source reads 3.1.2)

## 1.0.1 - 2026-08-27 - working

### Changed
- git tag v1.0.1 pushed (source at that tag still reads 3.1.1 - tag renamed retroactively)

## 1.0.0 - 2026-08-27 - failed

### Known
- The main package bundles an Untarnished UI SMF theme inside itself (SMF Theme - Untarnished UI 1.0.0\SKSE\plugins\SKSEMenuFrameworkThemes\UntarnishedUI-SMFTheme.json). That is base material carrying an optional theme's identity, the same problem found in the banners. Flagged for a decision, not changed - it alters an already-published package.
- IconDisplayExtensionArt.swf is MARKER artwork, not frame artwork - its symbols are TeammateMarker, NeutralMarker, hIconClip. The Untarnished UI variant differs from the main one by exactly 27 bytes across the whole 41,029-byte decompressed body: 9 colour records, #FFFFFF->#F5F2E9 (x4), #969696->#908E89 (x4), #B4B4B4->#ADABA4 (x1). It is a pure palette shift and needs no Flash toolchain to reproduce - the SWF is CWS/zlib and unpacks with zlib alone.
- Dragon's Eye Minimap refused to start against it (GetPluginInfo version gate); tag v1.0.0 kept in history but never for distribution

