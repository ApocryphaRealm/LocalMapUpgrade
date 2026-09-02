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

		// Draws a frame around the local map's rectangle.
		// OFF by default (the author, 2026-09-01): the game already draws its own frame round the
		// local map, and most UI replacers draw one too, so switching ours on unasked would stack
		// a second frame on top of somebody else's. It is a thing you turn ON because you want it -
		// typically because your replacer removed the vanilla frame - not a thing you have to
		// discover in order to turn off. One switch on the settings page enables it.
		inline bool localMapBorder = false;
		// The FRAME AROUND THE LOCAL MAP is a rectangle this mod draws itself, anchored from the
		// centre of the stage - there is no map background or frame art in the menu to reskin, so
		// the style is whatever we stroke (the author, 2026-09-01). 0 = Skyrim (the DEFAULT, and the standing rule for every UI element we draw: the
		// Nordic double line with a knot at each corner, matching the framework's Skyrim theme),
		// 1 = Untarnished (the plain single line this mod shipped with). The Skyrim style places
		// the REAL frame art shipped beside this DLL, not a drawing of it.
		inline std::uint32_t localMapBorderStyle = 0;
	}
}
