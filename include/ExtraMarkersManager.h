#pragma once

#include "Settings.h"

namespace LMU
{
	struct ExtraMarker
	{
		enum Type
		{
			kEnemy,
			kHostile,
			kGuard,
			kDead,
			kTeammate,
			kNeutral,
			kTotal
		};

		struct CreateData
		{
			enum
			{
				kName,
				kIconType,
				kCreateUndiscovered,
				kStride
			};
		};

		struct RefreshData
		{
			enum
			{
				kX,
				kY,
				kStride
			};
		};
	};

	class ExtraMarkersManager
	{
	public:
		static constexpr inline std::string_view extensionPath = "_level0.WorldMap.LocalMapMenu.IconDisplayExtension";
		static constexpr inline float feetToUnits = 21.3333333F;

		static void InitSingleton()
		{
			static ExtraMarkersManager instance;
			singleton = &instance;
		}

		static ExtraMarkersManager* GetSingleton() { return singleton; }

		void AddExtraMarkers(RE::LocalMapMenu& a_localMapMenu);

		// Draws a thin border around the local map's rectangle, in Untarnished UI's off-white,
		// when bLocalMapBorder is on. The rectangle comes from LocalMapMenu's own topLeft and
		// bottomRight, so it tracks the map wherever the game puts it rather than being a fixed
		// shape in the artwork - which is why this is here and not in the SWF.
		void DrawMapBorder(RE::LocalMapMenu& a_localMapMenu);

		void PostCreateMarkers(RE::GFxValue& a_iconDisplay);

		std::uint32_t GetAliveActorsDisplayRadius() const { return aliveActorsDisplayRadius / feetToUnits; }
		std::uint32_t GetUndeadActorsDisplayRadius() const { return undeadActorsDisplayRadius / feetToUnits; }
		std::uint32_t GetDeadActorsDisplayRadius() const { return deadActorsDisplayRadius / feetToUnits; }

		void SetAliveActorsDisplayRadius(std::uint32_t a_radius) { aliveActorsDisplayRadius = a_radius * feetToUnits; }
		void SetUndeadActorsDisplayRadius(std::uint32_t a_radius) { undeadActorsDisplayRadius = a_radius * feetToUnits; }
		void SetDeadActorsDisplayRadius(std::uint32_t a_radius) { deadActorsDisplayRadius = a_radius * feetToUnits; }

		// Mirrors the ternary the three radii above are constructed with (0 when immersive mode
		// is on, so no actor is ever in range until a Detect Life/Dead effect grows it back;
		// unlimited when it's off). The constructor only runs that ternary once, at
		// kDataLoaded - after settings::Init() has already read bImmersiveMode from the INI, so
		// changing the setting later (the settings menu, Reload, Restore Defaults) needs this to
		// actually take effect instead of leaving whatever radius the constructor captured.
		// Goes straight to the raw game-unit members rather than through the feet-based setters
		// above, since std::numeric_limits<std::uint32_t>::max() * feetToUnits would overflow.
		void SetImmersiveMode(bool a_enabled)
		{
			std::uint32_t radius = a_enabled ? 0 : std::numeric_limits<std::uint32_t>::max();

			aliveActorsDisplayRadius = radius;
			undeadActorsDisplayRadius = radius;
			deadActorsDisplayRadius = radius;
		}

	private:
		static void AddExtraMarker(RE::ActorHandle& a_actorHandle, RE::Actor* actor, RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers);

		static inline ExtraMarkersManager* singleton;

		std::uint32_t aliveActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ? 0 : std::numeric_limits<std::uint32_t>::max();
		std::uint32_t undeadActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ? 0 : std::numeric_limits<std::uint32_t>::max();
		std::uint32_t deadActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ? 0 : std::numeric_limits<std::uint32_t>::max();
	};
}