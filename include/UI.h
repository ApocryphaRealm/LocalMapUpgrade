#pragma once

namespace UI
{
	// Adds this mod's page to the SKSE Menu Framework's Mod Control Panel. Safe to call when
	// the framework is missing or too old to drive: it logs why and does nothing else.
	void Register();

	// Pushes the current values of settings::mapmenu and settings::debug into the running
	// game: the local map's pixel shader (color/fog-of-war/shape) and the log level. The
	// actor-visibility and pan-speed settings are read live wherever they're used, so they
	// need nothing doing here.
	void ApplyLiveSettings();

	namespace SettingsPanel
	{
		void __stdcall Render();
	}
}
