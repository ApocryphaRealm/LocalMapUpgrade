#pragma once

#include "utils/Logger.h"

#ifdef MessageBox
#define MessageBox_BAK MessageBox
#undef MessageBox
#endif

namespace LMU
{
	namespace detail
	{
		// RE::GameSettingCollection::GetSetting returns null for a name that is not a
		// registered GMST, and the engine's own callers dereference that unconditionally.
		// A raw ->data.s off a null Setting* is exactly the null-pointer-read class of crash
		// that shipped in Dragon's Eye Minimap - fall back to a placeholder string and log
		// so a missing/renamed GMST surfaces in the log instead of crashing at startup.
		inline const char* GetGMSTString(const char* a_name, const char* a_fallback)
		{
			RE::Setting* setting = RE::GameSettingCollection::GetSingleton()->GetSetting(a_name);
			if (!setting)
			{
				logger::warn("Game setting \"{}\" not found; using a fallback string", a_name);

				return a_fallback;
			}

			return setting->data.s;
		}
	}

	class PlayerSetMarkerManager
	{
		struct MessageBox
		{
			struct Callback : RE::IMessageBoxCallback
			{
				void Run(Message a_optionIndex) final;

				void SetData(RE::LocalMapMenu* a_localMapMenu, float a_wndPointX, float a_wndPointY)
				{
					localMapMenu = a_localMapMenu;
					wndPointX = a_wndPointX;
					wndPointY = a_wndPointY;
				}

				RE::LocalMapMenu* localMapMenu = nullptr;
				float wndPointX = 0.0F;
				float wndPointY = 0.0F;
			};

			MessageBox()
			{
				options.push_back(detail::GetGMSTString("sMoveMarker", "Move"));
				options.push_back(detail::GetGMSTString("sLeaveMarker", "Leave"));
				options.push_back(detail::GetGMSTString("sRemoveMarker", "Remove"));
			}

			RE::BSString title = detail::GetGMSTString("sMoveMarkerQuestion", "What do you want to do with this marker?");
			RE::BSTArray<RE::BSString> options;
			RE::BSTSmartPointer<Callback> callback = RE::make_smart<Callback>();
		};

	public:
		static PlayerSetMarkerManager* GetSingleton()
		{
			static PlayerSetMarkerManager singleton;
			return &singleton;
		}

		bool CanPlaceMarker() const { return allowPlaceMarker; }
		void AllowPlaceMarker() { allowPlaceMarker = true; }

		void PlaceMarker(RE::LocalMapMenu* a_localMapMenu, float a_wndPointX, float a_wndPointY);

	private:
		MessageBox messageBox;
		bool allowPlaceMarker = true;
	};
}

#ifdef MessageBox_BAK
#define MessageBox MessageBox_BAK
#undef MessageBox_BAK
#endif
