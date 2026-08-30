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
> * Numbers are assigned by the tooling, never by hand: mods via `version-ledger.ps1 -Action next`
>   then `set-version.ps1`; governed documents via `docs-pipeline.ps1 -Action bump`; the rules via
>   `rules-version.ps1 -Action bump`. If a number was typed by hand, it is wrong until the tool
>   agrees.

## 1.2.6 - 2026-08-27 - untested

### Fixed
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

