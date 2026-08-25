#include "Settings.h"

#include "utils/INISettingCollection.h"
#include "utils/Logger.h"

#include <windows.h>

namespace settings
{
	using namespace utils;

	namespace
	{
		constexpr const char* kDebugSection = "Debug";
		constexpr const char* kMapMenuSection = "MapMenu";

		std::string iniPath;
		std::string iniFileName;

		// The values the plugin compiles in, captured before the INI is read so that
		// "Restore defaults" means "what you would get with no INI at all".
		struct Defaults
		{
			logger::level logLevel;

			bool localMapColor;
			bool localMapFogOfWar;
			float localMapKeyboardPanSpeed;
			bool localMapShowEnemyActors;
			bool localMapShowHostileActors;
			bool localMapShowGuardActors;
			bool localMapShowDeadActors;
			bool localMapShowTeammateActors;
			bool localMapShowNeutralActors;
			bool localMapShowActorsOnlyWithDetectSpell;
		};

		Defaults defaults;

		void CaptureDefaults()
		{
			defaults.logLevel = debug::logLevel;

			defaults.localMapColor = mapmenu::localMapColor;
			defaults.localMapFogOfWar = mapmenu::localMapFogOfWar;
			defaults.localMapKeyboardPanSpeed = mapmenu::localMapKeyboardPanSpeed;
			defaults.localMapShowEnemyActors = mapmenu::localMapShowEnemyActors;
			defaults.localMapShowHostileActors = mapmenu::localMapShowHostileActors;
			defaults.localMapShowGuardActors = mapmenu::localMapShowGuardActors;
			defaults.localMapShowDeadActors = mapmenu::localMapShowDeadActors;
			defaults.localMapShowTeammateActors = mapmenu::localMapShowTeammateActors;
			defaults.localMapShowNeutralActors = mapmenu::localMapShowNeutralActors;
			defaults.localMapShowActorsOnlyWithDetectSpell = mapmenu::localMapShowActorsOnlyWithDetectSpell;
		}

		// WritePrivateProfileString rewrites a single key in place, so the comments and any
		// keys this plugin does not know about survive a save untouched.
		bool WriteRaw(const char* a_section, const char* a_key, const std::string& a_value)
		{
			if (::WritePrivateProfileStringA(a_section, a_key, a_value.c_str(), iniPath.c_str()))
			{
				return true;
			}

			logger::error("Could not write {}={} to {} (error {})", a_key, a_value, iniPath, ::GetLastError());

			return false;
		}

		bool WriteFloat(const char* a_section, const char* a_key, float a_value)
		{
			return WriteRaw(a_section, a_key, std::format("{:g}", a_value));
		}

		bool WriteUInt(const char* a_section, const char* a_key, std::uint32_t a_value)
		{
			return WriteRaw(a_section, a_key, std::format("{}", a_value));
		}

		bool WriteBool(const char* a_section, const char* a_key, bool a_value)
		{
			return WriteRaw(a_section, a_key, a_value ? "1" : "0");
		}

		// RE::INISettingCollection::GetSetting returns null for a name that is not in the
		// collection, and the templated GetSetting<T> helpers dereference that without
		// checking. AddChecked below deliberately skips a malformed setting, so a skipped one
		// would then be read back as null and crash during SKSEPluginLoad - trading one fatal
		// bug for another. Read through here instead: the value keeps whatever default it
		// already had, and the log says which setting went missing.
		template <typename T>
		T Read(INISettingCollection* a_collection, const char* a_name, T a_fallback)
		{
			if (!a_collection->GetSetting(a_name))
			{
				logger::error("Setting \"{}\" is missing from the collection; keeping the current value", a_name);

				return a_fallback;
			}

			return a_collection->GetSetting<T>(a_name);
		}

		// MakeSetting takes the setting's type from the first letter of its name - i signed,
		// u unsigned, f float, b bool, s string - and quietly hands back a setting with a null
		// name when the value passed does not match. The game's collection dereferences that
		// name, so inserting one crashes on startup with nothing useful in the log. Refuse it
		// here instead, where the message can say which setting is at fault.
		void AddChecked(INISettingCollection* a_collection, RE::Setting* a_setting, const char* a_name)
		{
			if (a_setting && a_setting->name)
			{
				a_collection->AddSettings(a_setting);

				return;
			}

			logger::critical("Setting \"{}\" was built with a value that does not match the type its "
							 "name prefix promises, so it has been skipped", a_name);
		}

		// Copies whatever the collection currently holds into the variables above. Shared by
		// Init() and Reload() so the two cannot read the INI differently.
		void ReadFromCollection()
		{
			INISettingCollection* c = INISettingCollection::GetSingleton();

			{
				using namespace debug;
				const auto raw = Read<std::uint32_t>(c, "uLogLevel:Debug", static_cast<std::uint32_t>(logLevel));

				// spdlog indexes its level table by this value, so a hand-edited uLogLevel=99
				// would read off the end of it the next time anything logged.
				logLevel = raw <= static_cast<std::uint32_t>(logger::level::off)
							   ? static_cast<logger::level>(raw)
							   : logger::level::info;
			}

			{
				using namespace mapmenu;
				localMapColor = Read<bool>(c, "bLocalMapColor:MapMenu", localMapColor);
				localMapFogOfWar = Read<bool>(c, "bLocalMapFogOfWar:MapMenu", localMapFogOfWar);
				localMapKeyboardPanSpeed = Read<float>(c, "fMapLocalKeyboardPanSpeed:MapMenu", localMapKeyboardPanSpeed);
				localMapShowEnemyActors = Read<bool>(c, "bMapLocalShowEnemyActors:MapMenu", localMapShowEnemyActors);
				localMapShowHostileActors = Read<bool>(c, "bMapLocalShowHostileActors:MapMenu", localMapShowHostileActors);
				localMapShowGuardActors = Read<bool>(c, "bMapLocalShowGuardActors:MapMenu", localMapShowGuardActors);
				localMapShowDeadActors = Read<bool>(c, "bMapLocalShowDeadActors:MapMenu", localMapShowDeadActors);
				localMapShowTeammateActors = Read<bool>(c, "bMapLocalShowTeammateActors:MapMenu", localMapShowTeammateActors);
				localMapShowNeutralActors = Read<bool>(c, "bMapLocalShowNeutralActors:MapMenu", localMapShowNeutralActors);
				localMapShowActorsOnlyWithDetectSpell = Read<bool>(c, "bImmersiveMode:MapMenu", localMapShowActorsOnlyWithDetectSpell);
			}
		}
	}

	void Init(const std::string& a_iniFileName)
	{
		CaptureDefaults();

		iniFileName = a_iniFileName;
		iniPath = std::filesystem::current_path().append("Data\\SKSE\\Plugins").append(a_iniFileName).string();

		INISettingCollection* iniSettingCollection = INISettingCollection::GetSingleton();

		// Registered one at a time through AddChecked, so a type that does not match its name
		// prefix is reported rather than crashing the game as it loads.
		const auto add = [iniSettingCollection](const char* a_name, auto a_value) {
			AddChecked(iniSettingCollection, MakeSetting(a_name, a_value), a_name);
		};

		{
			using namespace debug;
			add("uLogLevel:Debug", static_cast<std::uint32_t>(logLevel));
		}

		{
			using namespace mapmenu;
			add("bLocalMapColor:MapMenu", localMapColor);
			add("bLocalMapFogOfWar:MapMenu", localMapFogOfWar);
			add("fMapLocalKeyboardPanSpeed:MapMenu", localMapKeyboardPanSpeed);
			add("bMapLocalShowEnemyActors:MapMenu", localMapShowEnemyActors);
			add("bMapLocalShowHostileActors:MapMenu", localMapShowHostileActors);
			add("bMapLocalShowGuardActors:MapMenu", localMapShowGuardActors);
			add("bMapLocalShowDeadActors:MapMenu", localMapShowDeadActors);
			add("bMapLocalShowTeammateActors:MapMenu", localMapShowTeammateActors);
			add("bMapLocalShowNeutralActors:MapMenu", localMapShowNeutralActors);
			add("bImmersiveMode:MapMenu", localMapShowActorsOnlyWithDetectSpell);
		}

		if (!iniSettingCollection->ReadFromFile(a_iniFileName))
		{
			logger::warn("Could not read {}, falling back to default options", a_iniFileName);
		}

		ReadFromCollection();
	}

	bool Reload()
	{
		if (iniFileName.empty())
		{
			logger::error("Cannot reload settings before Init() has run");

			return false;
		}

		if (!INISettingCollection::GetSingleton()->ReadFromFile(iniFileName))
		{
			logger::error("Could not re-read {}; keeping the settings already loaded", iniPath);

			return false;
		}

		ReadFromCollection();

		logger::info("Reloaded settings from {}", iniPath);

		return true;
	}

	bool Save()
	{
		if (iniPath.empty())
		{
			logger::error("Cannot save settings before Init() has run");

			return false;
		}

		bool ok = true;

		ok &= WriteUInt(kDebugSection, "uLogLevel", static_cast<std::uint32_t>(debug::logLevel));

		ok &= WriteBool(kMapMenuSection, "bLocalMapColor", mapmenu::localMapColor);
		ok &= WriteBool(kMapMenuSection, "bLocalMapFogOfWar", mapmenu::localMapFogOfWar);
		ok &= WriteFloat(kMapMenuSection, "fMapLocalKeyboardPanSpeed", mapmenu::localMapKeyboardPanSpeed);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowEnemyActors", mapmenu::localMapShowEnemyActors);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowHostileActors", mapmenu::localMapShowHostileActors);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowGuardActors", mapmenu::localMapShowGuardActors);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowDeadActors", mapmenu::localMapShowDeadActors);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowTeammateActors", mapmenu::localMapShowTeammateActors);
		ok &= WriteBool(kMapMenuSection, "bMapLocalShowNeutralActors", mapmenu::localMapShowNeutralActors);
		ok &= WriteBool(kMapMenuSection, "bImmersiveMode", mapmenu::localMapShowActorsOnlyWithDetectSpell);

		// Flush the cached INI writes so the file on disk is up to date even if the game is
		// closed the hard way straight afterwards.
		::WritePrivateProfileStringA(nullptr, nullptr, nullptr, iniPath.c_str());

		if (ok)
		{
			logger::info("Saved settings to {}", iniPath);
		}

		return ok;
	}

	void RestoreDefaults()
	{
		debug::logLevel = defaults.logLevel;

		mapmenu::localMapColor = defaults.localMapColor;
		mapmenu::localMapFogOfWar = defaults.localMapFogOfWar;
		mapmenu::localMapKeyboardPanSpeed = defaults.localMapKeyboardPanSpeed;
		mapmenu::localMapShowEnemyActors = defaults.localMapShowEnemyActors;
		mapmenu::localMapShowHostileActors = defaults.localMapShowHostileActors;
		mapmenu::localMapShowGuardActors = defaults.localMapShowGuardActors;
		mapmenu::localMapShowDeadActors = defaults.localMapShowDeadActors;
		mapmenu::localMapShowTeammateActors = defaults.localMapShowTeammateActors;
		mapmenu::localMapShowNeutralActors = defaults.localMapShowNeutralActors;
		mapmenu::localMapShowActorsOnlyWithDetectSpell = defaults.localMapShowActorsOnlyWithDetectSpell;
	}

	const std::string& GetIniPath() { return iniPath; }
}
