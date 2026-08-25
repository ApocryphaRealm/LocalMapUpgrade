#include "UI.h"

#include "SKSEMenuFramework.h"

#include "ExtraMarkersManager.h"
#include "ShaderManager.h"
#include "Settings.h"

#include "utils/Logger.h"

namespace UI
{
	namespace
	{
		std::string statusMessage;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
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
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth"
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

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("(?)");

			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		void RenderMapMenuSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText("Local map");

			if (ImGuiMCP::Checkbox("Color", &mapmenu::localMapColor))
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
			HelpMarker("Renders the local (dungeon/interior) map in color instead of the vanilla black-and-white.");

			if (ImGuiMCP::Checkbox("Fog of war", &mapmenu::localMapFogOfWar))
			{
				OnMainThread([]() {
					if (auto* shaderManager = LMU::ShaderManager::GetSingleton())
					{
						shaderManager->SetFogOfWar(settings::mapmenu::localMapFogOfWar);
					}
				});
			}
			HelpMarker("Whether unexplored parts of the local map stay hidden. Disabling reveals the whole map.");

			ImGuiMCP::SliderFloat("Keyboard pan speed", &mapmenu::localMapKeyboardPanSpeed, 5.0F, 300.0F, "%.0f");
			HelpMarker("How fast the local map pans when panning it with the keyboard.");

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("Actor markers");

			ImGuiMCP::Checkbox("Show enemy actors", &mapmenu::localMapShowEnemyActors);
			ImGuiMCP::Checkbox("Show hostile actors", &mapmenu::localMapShowHostileActors);
			ImGuiMCP::Checkbox("Show guard actors", &mapmenu::localMapShowGuardActors);
			ImGuiMCP::Checkbox("Show dead actors", &mapmenu::localMapShowDeadActors);
			ImGuiMCP::Checkbox("Show teammate actors", &mapmenu::localMapShowTeammateActors);
			ImGuiMCP::Checkbox("Show neutral actors", &mapmenu::localMapShowNeutralActors);

			if (ImGuiMCP::Checkbox("Immersive mode", &mapmenu::localMapShowActorsOnlyWithDetectSpell))
			{
				if (auto* extraMarkersManager = LMU::ExtraMarkersManager::GetSingleton())
				{
					extraMarkersManager->SetImmersiveMode(mapmenu::localMapShowActorsOnlyWithDetectSpell);
				}
			}
			HelpMarker("Only shows actor markers on the local map while a detect life/dead effect is active, instead of always. Ships on by default - turn it off to always see actor markers.");
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText("Debug");

			int level = static_cast<int>(debug::logLevel);
			if (ImGuiMCP::Combo("Log level", &level, kLogLevelNames, kLogLevelCount))
			{
				debug::logLevel = static_cast<logger::level>(level);

				OnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });
			}
			HelpMarker("Applies to the log immediately.");
		}

		void RenderButtons()
		{
			if (ImGuiMCP::Button("Save"))
			{
				OnMainThread([]() {
					statusMessage = settings::Save() ? "Settings saved." : "Could not save the INI. See the log for why.";
				});
			}
			HelpMarker("Writes every setting above back to the INI. Comments and unrelated keys are left alone.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Reload from INI"))
			{
				OnMainThread([]() {
					if (settings::Reload())
					{
						ApplyLiveSettings();

						statusMessage = "Settings reloaded from the INI.";
					}
					else
					{
						statusMessage = "Could not read the INI. See the log for why.";
					}
				});
			}
			HelpMarker("Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Restore defaults"))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					ApplyLiveSettings();
				});

				statusMessage = "Defaults restored. Press Save to keep them.";
			}
			HelpMarker("Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.");

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
			logger::info("SKSE Menu Framework is not installed; settings will be read from the INI only");

			return;
		}

		if (!HasRequiredExports())
		{
			logger::warn("The installed SKSE Menu Framework is older than this plugin's settings "
						 "menu needs. Update it to version 3 or newer to configure the local map in game.");

			return;
		}

		SKSEMenuFramework::SetSection("Local Map Upgrade");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);

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
		ImGuiMCP::TextWrapped("Most settings apply as soon as you make them. Press Save to keep them for the next time you play.");
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
