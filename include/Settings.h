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
	}
}
