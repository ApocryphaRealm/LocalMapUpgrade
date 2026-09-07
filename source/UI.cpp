#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Diagnostics.h"

#include "ExtraMarkersManager.h"
#include "ShaderManager.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"
#include "utils/Toggle.h"

#include <algorithm>
#include <string>
#include <vector>

namespace UI
{
	namespace
	{
		std::string statusMessage;

		// The slider the arrow keys currently drive. Set by clicking one.
		std::string selectedSlider;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr const char* kLogLevelKeys[] = { "LMU_LogLevel_Trace", "LMU_LogLevel_Debug", "LMU_LogLevel_Info",
													"LMU_LogLevel_Warning", "LMU_LogLevel_Error", "LMU_LogLevel_Critical", "LMU_LogLevel_Off" };
		constexpr int kLogLevelCount = 7;

		// The framework renders from the renderer's present hook, which is not the thread
		// Scaleform and the rest of the game expect to be talked to. Anything that reaches into
		// the shader/map state has to be handed to the main thread first.
		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		// The bundled header reaches ImGui through the framework's exported cimgui entry points.
		// Older builds of SKSE Menu Framework do not export them, and every widget call in
		// Render() would then call through a null function pointer, so refuse to register
		// unless the ones this panel needs are all there. Varargs widgets resolve to a
		// "...V"-suffixed export (TextDisabled resolves igTextDisabledV, not igTextDisabled) -
		// probing the wrong name lets Register() succeed and then crashes on the first draw.
		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCheckbox",
				"igCombo_Str_arr",
				"igSliderFloat",
				// Needed by NudgeableSlider's arrow-key nudge (ported from Dragon's Eye
				// Minimap's UI.cpp - CLAUDE.md rule 24).
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				// Toggle() - the on/off switch every boolean setting now renders as
				// instead of a tick-box (utils/Toggle.h, CLAUDE.md rule 32).
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("SKSE Menu Framework does not export \"{}\"", name);

					return false;
				}
			}

			return true;
		}

		// A slider that the arrow keys can also nudge, once it has been clicked. Dragging is
		// hopeless for the last decimal place, and the framework does not turn on ImGui's own
		// keyboard navigation, so this tracks the selection itself rather than changing a
		// setting shared with every other mod's page. Ported verbatim from Dragon's Eye
		// Minimap's UI.cpp, which already had this working - see CLAUDE.md rule 24.
		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);

			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive())
			{
				selectedSlider = a_label;
			}

			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;

				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow))
				{
					nudge -= a_step;
				}
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow))
				{
					nudge += a_step;
				}

				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}

				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("%s", strings::TR("LMU_SliderNudge", "<-->"));
			}

			return changed;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", strings::TR("LMU_HelpMark", "(?)"));

			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		void RenderMapMenuSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("LMU_Title", "Local map"));

			if (ImGuiMCP::Toggle(strings::TR("LMU_Color", "Color"), &mapmenu::localMapColor))
			{
				OnMainThread([]() {
					if (auto* shaderManager = LMU::ShaderManager::GetSingleton())
					{
						LMU::PixelShaderProperty::Shape shape;
						LMU::PixelShaderProperty::Style unused;
						LMU::ShaderManager::GetPixelShaderProperties(shape, unused);

						LMU::ShaderManager::SetPixelShaderProperties(shape,
							settings::mapmenu::localMapColor ? LMU::PixelShaderProperty::Style::kColor
															  : LMU::PixelShaderProperty::Style::kBlackNWhite);
					}
				});
			}
			HelpMarker(strings::TR("LMU_HelpColor", "Renders the local (dungeon/interior) map in color instead of the vanilla black-and-white."));

			if (ImGuiMCP::Toggle(strings::TR("LMU_FogOfWar", "Fog of war"), &mapmenu::localMapFogOfWar))
			{
				OnMainThread([]() {
					if (auto* shaderManager = LMU::ShaderManager::GetSingleton())
					{
						shaderManager->SetFogOfWar(settings::mapmenu::localMapFogOfWar);
					}
				});
			}
			HelpMarker(strings::TR("LMU_HelpFogOfWar", "Whether unexplored parts of the local map stay hidden. Disabling reveals the whole map."));

			NudgeableSlider(strings::TR("LMU_KeyboardPanSpeed", "Keyboard pan speed"), &mapmenu::localMapKeyboardPanSpeed, 5.0F, 300.0F, "%.0f", 1.0F);
			HelpMarker(strings::TR("LMU_HelpKeyboardPanSpeed", "How fast the local map pans when panning it with the keyboard."));

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("%s", strings::TR("LMU_ActorMarkers", "Actor markers"));

			ImGuiMCP::Toggle(strings::TR("LMU_ShowEnemyActors", "Show enemy actors"), &mapmenu::localMapShowEnemyActors);
			ImGuiMCP::Toggle(strings::TR("LMU_ShowHostileActors", "Show hostile actors"), &mapmenu::localMapShowHostileActors);
			ImGuiMCP::Toggle(strings::TR("LMU_ShowGuardActors", "Show guard actors"), &mapmenu::localMapShowGuardActors);
			ImGuiMCP::Toggle(strings::TR("LMU_ShowDeadActors", "Show dead actors"), &mapmenu::localMapShowDeadActors);
			ImGuiMCP::Toggle(strings::TR("LMU_ShowTeammateActors", "Show teammate actors"), &mapmenu::localMapShowTeammateActors);
			ImGuiMCP::Toggle(strings::TR("LMU_ShowNeutralActors", "Show neutral actors"), &mapmenu::localMapShowNeutralActors);

			if (ImGuiMCP::Toggle(strings::TR("LMU_ImmersiveMode", "Immersive mode"), &mapmenu::localMapShowActorsOnlyWithDetectSpell))
			{
				if (auto* extraMarkersManager = LMU::ExtraMarkersManager::GetSingleton())
				{
					extraMarkersManager->SetImmersiveMode(mapmenu::localMapShowActorsOnlyWithDetectSpell);
				}
			}
			HelpMarker(strings::TR("LMU_HelpImmersiveMode", "Only shows actor markers on the local map while a detect life/dead effect is active, instead of always. Ships off by default - turn it on if you want markers gated behind a detect effect."));

			ImGuiMCP::Toggle(strings::TR("LMU_MapBorder", "Map border"), &mapmenu::localMapBorder);
			HelpMarker(strings::TR("LMU_HelpMapBorder", "Draws a frame around the local map. OFF by default: the game draws its own frame, and most UI replacers draw one too, so this would otherwise stack a second frame on somebody else's. Turn it on if your replacer removed the vanilla frame, or if you prefer ours. Applies live - no need to reopen the map."));

			if (mapmenu::localMapBorder)
			{
				// "Skyrim" and "Untarnished" are art names (and folder identities) - left untranslated,
				// same as the game's other UI-replacer theme option lists.
				static const char* const kBorderStyles[] = { "Skyrim", "Untarnished" };
				int style = static_cast<int>(mapmenu::localMapBorderStyle);
				if (style < 0 || style > 1) { style = 0; }
				if (ImGuiMCP::Combo(strings::TR("LMU_BorderStyle", "Border style"), &style, kBorderStyles, 2))
				{
					mapmenu::localMapBorderStyle = static_cast<std::uint32_t>(style);
				}
				HelpMarker(strings::TR("LMU_HelpBorderStyle", "Skyrim: the knotwork frame - the same Nordic art the menu framework's Skyrim theme uses, drawn round the map. The default. Untarnished: a plain single line in Untarnished UI's off-white."));
			}
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("LMU_Debug", "Debug"));

			int level = static_cast<int>(debug::logLevel);
			// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated
			// bytes for this call so the const char* pointers handed to Combo stay valid.
			std::vector<std::string> logLevelLabelStore;
			logLevelLabelStore.reserve(kLogLevelCount);
			for (int i = 0; i < kLogLevelCount; ++i)
			{
				logLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));
			}
			std::vector<const char*> logLevelLabels;
			logLevelLabels.reserve(logLevelLabelStore.size());
			for (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }
			if (ImGuiMCP::Combo(strings::TR("LMU_LogLevel", "Log level"), &level, logLevelLabels.data(), kLogLevelCount))
			{
				debug::logLevel = static_cast<logger::level>(level);

				OnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });
			}
			HelpMarker(strings::TR("LMU_HelpLogLevel", "Applies to the log immediately."));
		}

		void RenderButtons()
		{
			if (ImGuiMCP::Button(strings::TR("LMU_SaveBtn", "Save")))
			{
				OnMainThread([]() {
					statusMessage = settings::Save() ? strings::TR("LMU_StatusSaved", "Settings saved.")
													   : strings::TR("LMU_StatusSaveFail", "Could not save the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("LMU_HelpSave", "Writes every setting above back to the INI. Comments and unrelated keys are left alone."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("LMU_ReloadBtn", "Reload from INI")))
			{
				OnMainThread([]() {
					if (settings::Reload())
					{
						ApplyLiveSettings();

						statusMessage = strings::TR("LMU_StatusReloaded", "Settings reloaded from the INI.");
					}
					else
					{
						statusMessage = strings::TR("LMU_StatusReloadFail", "Could not read the INI. See the log for why.");
					}
				});
			}
			HelpMarker(strings::TR("LMU_HelpReload", "Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("LMU_RestoreBtn", "Restore defaults")))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					ApplyLiveSettings();
				});

				statusMessage = strings::TR("LMU_StatusRestored", "Defaults restored. Press Save to keep them.");
			}
			HelpMarker(strings::TR("LMU_HelpRestore", "Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save."));

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			// Recorded as "attempted but not registered" rather than left unrecorded: from in
			// game, a missing settings page and a settings page that failed to draw look the
			// same, and only one of them is this plugin's problem.
			diagnostics::RecordSettingsMenuRegistered(false);

			logger::info("SKSE Menu Framework is not installed; settings will be read from the INI only");

			return;
		}

		if (!HasRequiredExports())
		{
			diagnostics::RecordSettingsMenuRegistered(false);

			logger::warn("The installed SKSE Menu Framework is older than this plugin's settings "
						 "menu needs. Update it to version 3 or newer to configure the local map in game.");

			return;
		}

		SKSEMenuFramework::SetSection("Local Map Upgrade");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);

		diagnostics::RecordSettingsMenuRegistered(true);

		logger::info("Registered the settings page with SKSE Menu Framework");
	}

	void ApplyLiveSettings()
	{
		logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

		if (auto* extraMarkersManager = LMU::ExtraMarkersManager::GetSingleton())
		{
			extraMarkersManager->SetImmersiveMode(settings::mapmenu::localMapShowActorsOnlyWithDetectSpell);
		}

		OnMainThread([]() {
			if (auto* shaderManager = LMU::ShaderManager::GetSingleton())
			{
				LMU::PixelShaderProperty::Shape shape;
				LMU::PixelShaderProperty::Style unused;
				LMU::ShaderManager::GetPixelShaderProperties(shape, unused);

				LMU::ShaderManager::SetPixelShaderProperties(shape,
					settings::mapmenu::localMapColor ? LMU::PixelShaderProperty::Style::kColor
													  : LMU::PixelShaderProperty::Style::kBlackNWhite);

				shaderManager->SetFogOfWar(settings::mapmenu::localMapFogOfWar);
			}
		});
	}

	void __stdcall SettingsPanel::Render()
	{
		strings::Tick();

		ImGuiMCP::TextWrapped("%s", strings::TR("LMU_Intro", "Most settings apply as soon as you make them. Press Save to keep them for the next time you play."));
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderMapMenuSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
