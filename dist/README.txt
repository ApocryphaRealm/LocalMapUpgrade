# Local Map Upgrade

Version 1.3.2

WHAT CHANGED
------------

Version 1.3.2
Added a Skyrim 1.7 build (1.7.99 and later); the installer asks which game you run. Dragon's Eye Minimap's 1.7 build draws its minimap through it.
Version 1.3.1
The Address Library pre-check runs before anything else at load and names the missing file for your game version, instead of CommonLibSSE-NG's bare failure line.
Version 1.3.0
Relicensed to GPL-3.0-or-later as a whole; alexsylex's original MIT notice is kept in full in THIRD_PARTY_NOTICES.md. No gameplay change.
Version 1.2.9
The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL, and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback.
Version 1.2.7
Added a Border style setting that picks the frame drawn around the local map: Skyrim knotwork or Untarnished.
Changed the map border to ship off by default; turn it on with the Map border toggle if you want a frame.
Version 1.2.6
Fixed the settings menu never registering under Apocrypha Menu Framework, so the page was silently absent.
Fixed settings reverting to their values from game start on reload, and being overwritten between sessions, when PrivateProfileRedirector is installed.
Debug symbols now ship inside the main download so Crash Logger can resolve this mod's crash frames.
Version 1.2.5
Reverted an arrow-key change from the previous build that stopped a slider nudge from also moving gamepad menu navigation elsewhere on screen. Sliders can still be nudged with all four arrow keys.
Version 1.2.4
Changed the on/off toggle switch to turn green when on and red when off.
The pan speed slider now also responds to the up and down arrow keys, not just left and right.
Version 1.2.2
The pan speed slider can now be nudged with the arrow keys - click to select it, then any of the four arrow keys nudges its value.
Version 1.2.1
Fixed the border sitting a few pixels inside the map on the left and top edges.
Fixed the border appearing inside a minimap mod's own map instead of only the Local Map screen.
Changed how the border's Flash clip is created so it can no longer silently delete part of another UI mod sharing the same depth.
Version 1.2.0
Fixed the border finally framing the correct rectangle, the black plate the map sits on, anchored from the centre of the map screen so it stays correct at any resolution.
Version 1.1.9
Diagnostic build only; no visible change.
Version 1.1.8
Another attempt at placing the border; still not correctly positioned.
Version 1.1.7
Changed how the border is placed, from the map's own reported extents rather than a Scaleform clip.
Version 1.1.6
The border is now on by default; turn it off in the settings page if you prefer the unframed look.
Widened the border to match the black plate the map sits on, rather than the smaller inset render area.
Version 1.1.5
Fixed the border never appearing on the actual Local Map screen while turning up inside Dragon's Eye Minimap instead.
Version 1.1.4
Fixed the border drawing inside Dragon's Eye Minimap's own map.
Version 1.1.3
Fixed the border appearing far off toward the bottom-right corner of the screen instead of around the map.
Version 1.1.2
Actor markers are now flat, with no shading.
Version 1.1.1
Actor markers are now brighter and fully opaque.
Version 1.1.0
Added an optional thin off-white border around the local map, matching Untarnished UI's palette. Off by default in this version.
Version 1.0.9
Fixed the log growing enormously while the local map was open; lines are now written only when something actually changes.
Version 1.0.8
Logging now defaults to its most detailed level, so a first bug report is useful without reproducing the problem again.
Version 1.0.7
Immersive mode now ships off instead of on.
Version 1.0.6
Version renumbering only; no functional change.
Version 1.0.5
Safety audit across the plugin. No known crash was found; this was preventative.
Version 1.0.4
Fixed Immersive mode not taking effect when toggled from the settings menu; it only applied on startup before.
Version 1.0.3
Fixed the Colour and Fog of war settings using the wrong INI key names, so a saved value for either was ignored at startup.
Version 1.0.2
Fixed the Colour setting not reaching Dragon's Eye Minimap.
Version 1.0.1
Version fix so Dragon's Eye Minimap's dependency check was satisfied.
Version 1.0.0
First release. Adds an in-game settings page through SKSE Menu Framework, so the local map can be configured while playing instead of only by editing the INI before launching.
