#pragma once

namespace SKSE::log
{
	using level = spdlog::level::level_enum;
}
namespace logger = SKSE::log;

namespace settings
{
	// Reads the INI into the variables below. The values the variables hold when this is
	// called are remembered as the built-in defaults, so RestoreDefaults() can put them back.
	void Init(const std::string& a_iniFileName);

	// Writes every setting below back to the INI that Init() read, leaving the comments and
	// any unrelated keys in that file alone. Returns false if the file could not be written.
	bool Save();

	// Puts every setting back to its built-in default. This only touches the variables;
	// follow it with Save() to persist, and with UI::ApplyLiveSettings() to show it in game.
	void RestoreDefaults();

	// Re-reads the INI that Init() read, discarding any unsaved change made since. Returns
	// false if the file could not be read, leaving the current values alone. Follow it with
	// UI::ApplyLiveSettings() to show the reloaded values in game.
	bool Reload();

	// Full path of the INI Init() read, or an empty string before Init() has run.
	const std::string& GetIniPath();

	namespace debug
	{
		inline logger::level logLevel = logger::level::trace;
	}

	namespace mapmenu
	{
		inline bool localMapColor = true;
		inline bool localMapFogOfWar = true;
		inline float localMapKeyboardPanSpeed = 60.0F;
		inline bool localMapShowEnemyActors = true;
		inline bool localMapShowHostileActors = true;
		inline bool localMapShowGuardActors = true;
		inline bool localMapShowDeadActors = true;
		inline bool localMapShowTeammateActors = true;
		inline bool localMapShowNeutralActors = true;
		inline bool localMapShowActorsOnlyWithDetectSpell = false;

		// Draws a thin border around the local map's rectangle, in Untarnished UI's off-white.
		// On by default: it frames the map cleanly at any resolution and reads as part of the
		// map rather than as an addition, so it is worth showing without being asked for.
		// Anyone who does not want it can turn it off in the settings menu.
		inline bool localMapBorder = true;
		// The FRAME AROUND THE LOCAL MAP is a rectangle this mod draws itself, anchored from the
		// centre of the stage - there is no map background or frame art in the menu to reskin, so
		// the style is whatever we stroke (the author, 2026-09-01). 0 = Skyrim (the DEFAULT, and the standing rule for every UI element we draw: the
		// Nordic double line with a knot at each corner, matching the framework's Skyrim theme),
		// 1 = Untarnished (the plain single line this mod shipped with). The Skyrim style places
		// the REAL frame art shipped beside this DLL, not a drawing of it.
		inline std::uint32_t localMapBorderStyle = 0;
	}
}
