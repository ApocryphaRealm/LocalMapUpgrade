#include "Diagnostics.h"

#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <mutex>

namespace diagnostics
{
	namespace
	{
		using clock = std::chrono::steady_clock;

		std::mutex mtx;

		constexpr const char* hookNames[static_cast<std::size_t>(Hook::kTotal)] = {
			"isSmallWorld",
			"addQuestMarkersToMap",
			"invokeCreateMarkers",
			"inputHandlerCanProcess",
			"inputHandlerProcessButton",
			"waterShaderSetupTechnique",
			"detectLifeEffectUpdate",
			"scriptEffectUpdate",
			"stopHitEffectsVisit",
			"shaderReferenceEffectDetach",
			"toggleFogOfWarCommand"
		};

		// Eight ISLocalMap.hlsl variants: squared/round x colour/black-and-white x fog/no-fog.
		constexpr std::uint32_t expectedShaderVariants = 8;

		struct State
		{
			// Hook installation. attempted[] separates "installed and failed" from "never got
			// there at all", which is the difference between a bad address and an early return.
			bool hookAttempted[static_cast<std::size_t>(Hook::kTotal)] = {};
			bool hookInstalled[static_cast<std::size_t>(Hook::kTotal)] = {};

			// Startup milestones.
			bool infinityUIListenerAttempted = false;
			bool infinityUIListenerRegistered = false;
			bool iconDisplayExtensionPatched = false;
			bool shaderManagerInitialized = false;
			bool extraMarkersManagerInitialized = false;
			bool settingsMenuAttempted = false;
			bool settingsMenuRegistered = false;

			// Shader.
			bool localMapShaderChecked = false;
			bool localMapShaderFound = false;
			std::uint32_t shaderVariantsCreated = 0;
			std::uint32_t shaderVariantsFailed = 0;
			std::string lastShaderFailureReason;

			bool havePixelShaderProperties = false;
			bool shapeRound = false;
			bool styleColor = false;
			bool fogOfWarEnabled = false;
			std::uint64_t pixelShaderPropertiesSets = 0;
			std::optional<clock::time_point> lastPixelShaderPropertiesSet;
			std::uint64_t pixelShaderPropertiesReads = 0;
			std::optional<clock::time_point> lastPixelShaderPropertiesRead;

			bool pixelShaderPropertiesHookDispatched = false;
			std::optional<clock::time_point> lastPixelShaderPropertiesHookDispatch;

			// Local map activity.
			std::uint64_t localMapFrames = 0;
			std::optional<clock::time_point> lastLocalMapFrame;
			std::uint64_t localMapOpens = 0;
			std::optional<clock::time_point> lastLocalMapOpen;
			bool haveLastFrame = false;
			LocalMapFrame lastFrame;
			std::uint32_t peakMarkersAdded = 0;

			std::uint64_t extraMarkerDataNotReadyFrames = 0;
			std::optional<clock::time_point> lastExtraMarkerDataNotReady;

			std::uint64_t mapBorderDraws = 0;
			std::optional<clock::time_point> lastMapBorderDraw;

			// Player-set marker.
			std::uint64_t playerMarkerPrompts = 0;
			std::optional<clock::time_point> lastPlayerMarkerPrompt;
			std::uint64_t playerMarkersPlaced = 0;
			std::optional<clock::time_point> lastPlayerMarkerPlaced;
			std::uint64_t playerMarkersRemoved = 0;
			std::optional<clock::time_point> lastPlayerMarkerRemoved;
			std::uint64_t playerMarkerPlaceFailures = 0;
			std::optional<clock::time_point> lastPlayerMarkerPlaceFailure;
			std::string lastPlayerMarkerPlaceFailureReason;
		};

		State state;

		// Escapes the handful of characters JSON requires. Most strings this module emits are
		// fixed literals from our own code (a failure reason), but the INI path is a real
		// filesystem path full of backslashes - there this is load-bearing, not defensive.
		std::string EscapeJson(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size());

			for (char c : a_text)
			{
				switch (c)
				{
				case '"':
					out += "\\\"";
					break;
				case '\\':
					out += "\\\\";
					break;
				case '\n':
					out += "\\n";
					break;
				case '\r':
					out += "\\r";
					break;
				case '\t':
					out += "\\t";
					break;
				default:
					out += c;
					break;
				}
			}

			return out;
		}

		// Renders "field": null or "field": <seconds ago>, so a query can tell "never happened"
		// apart from "happened a long time ago" instead of both looking like a missing/zero field.
		std::string SecondsAgoField(const char* a_name, const std::optional<clock::time_point>& a_when)
		{
			if (!a_when)
			{
				return std::format("\"{}SecondsAgo\": null", a_name);
			}

			const double seconds = std::chrono::duration<double>(clock::now() - *a_when).count();

			return std::format("\"{}SecondsAgo\": {:.1f}", a_name, seconds);
		}

		const char* Bool(bool a_value) { return a_value ? "true" : "false"; }

		std::string SettingsSection()
		{
			const spdlog::string_view_t levelName = spdlog::level::to_string_view(settings::debug::logLevel);

			return std::format(
				"\"settings\":{{"
				"\"iniPath\":\"{}\","
				"\"logLevel\":\"{}\","
				"\"localMapColor\":{},"
				"\"localMapFogOfWar\":{},"
				"\"localMapBorder\":{},"
				"\"localMapKeyboardPanSpeed\":{:.2f},"
				"\"showEnemyActors\":{},"
				"\"showHostileActors\":{},"
				"\"showGuardActors\":{},"
				"\"showDeadActors\":{},"
				"\"showTeammateActors\":{},"
				"\"showNeutralActors\":{},"
				"\"showActorsOnlyWithDetectSpell\":{}"
				"}}",
				EscapeJson(settings::GetIniPath()),
				EscapeJson(std::string_view(levelName.data(), levelName.size())),
				Bool(settings::mapmenu::localMapColor),
				Bool(settings::mapmenu::localMapFogOfWar),
				Bool(settings::mapmenu::localMapBorder),
				settings::mapmenu::localMapKeyboardPanSpeed,
				Bool(settings::mapmenu::localMapShowEnemyActors),
				Bool(settings::mapmenu::localMapShowHostileActors),
				Bool(settings::mapmenu::localMapShowGuardActors),
				Bool(settings::mapmenu::localMapShowDeadActors),
				Bool(settings::mapmenu::localMapShowTeammateActors),
				Bool(settings::mapmenu::localMapShowNeutralActors),
				Bool(settings::mapmenu::localMapShowActorsOnlyWithDetectSpell));
		}

		// Caller holds mtx.
		std::string StartupSection()
		{
			return std::format(
				"\"startup\":{{"
				"\"infinityUIListenerAttempted\":{},"
				"\"infinityUIListenerRegistered\":{},"
				"\"iconDisplayExtensionPatched\":{},"
				"\"shaderManagerInitialized\":{},"
				"\"extraMarkersManagerInitialized\":{},"
				"\"settingsMenuAttempted\":{},"
				"\"settingsMenuRegistered\":{}"
				"}}",
				Bool(state.infinityUIListenerAttempted),
				Bool(state.infinityUIListenerRegistered),
				Bool(state.iconDisplayExtensionPatched),
				Bool(state.shaderManagerInitialized),
				Bool(state.extraMarkersManagerInitialized),
				Bool(state.settingsMenuAttempted),
				Bool(state.settingsMenuRegistered));
		}

		// Caller holds mtx.
		std::string HooksSection()
		{
			std::string detail;
			std::string notInstalled;
			std::uint32_t installedCount = 0;

			for (std::size_t i = 0; i < static_cast<std::size_t>(Hook::kTotal); ++i)
			{
				if (!detail.empty())
				{
					detail += ',';
				}

				// "installed" / "failed" / "notAttempted" says more than a bare bool would: the
				// third state means Install() never reached that line at all.
				const char* status = state.hookInstalled[i] ? "installed" : (state.hookAttempted[i] ? "failed" : "notAttempted");

				detail += std::format("\"{}\":\"{}\"", hookNames[i], status);

				if (state.hookInstalled[i])
				{
					++installedCount;
				}
				else
				{
					if (!notInstalled.empty())
					{
						notInstalled += ',';
					}

					notInstalled += std::format("\"{}\"", hookNames[i]);
				}
			}

			return std::format(
				"\"hooks\":{{"
				"\"allInstalled\":{},"
				"\"installedCount\":{},"
				"\"totalCount\":{},"
				"\"notInstalled\":[{}],"
				"\"detail\":{{{}}}"
				"}}",
				Bool(installedCount == static_cast<std::uint32_t>(Hook::kTotal)),
				installedCount,
				static_cast<std::uint32_t>(Hook::kTotal),
				notInstalled,
				detail);
		}

		// Caller holds mtx.
		std::string ShaderSection()
		{
			std::string properties;

			if (state.havePixelShaderProperties)
			{
				properties = std::format(
					"\"shape\":\"{}\",\"style\":\"{}\",\"fogOfWarEnabled\":{}",
					state.shapeRound ? "round" : "squared",
					state.styleColor ? "color" : "blackAndWhite",
					Bool(state.fogOfWarEnabled));
			}
			else
			{
				properties = "\"shape\":null,\"style\":null,\"fogOfWarEnabled\":null";
			}

			return std::format(
				"\"shader\":{{"
				"\"localMapShaderChecked\":{},"
				"\"localMapShaderFound\":{},"
				"\"variantsCreated\":{},"
				"\"variantsFailed\":{},"
				"\"variantsExpected\":{},"
				"\"allVariantsReady\":{},"
				"\"lastFailureReason\":\"{}\","
				"{},"
				"\"propertiesSetCount\":{},"
				"{},"
				"\"propertiesReadCount\":{},"
				"{}"
				"}}",
				Bool(state.localMapShaderChecked),
				Bool(state.localMapShaderFound),
				state.shaderVariantsCreated,
				state.shaderVariantsFailed,
				expectedShaderVariants,
				Bool(state.localMapShaderFound && state.shaderVariantsCreated == expectedShaderVariants),
				EscapeJson(state.lastShaderFailureReason),
				properties,
				state.pixelShaderPropertiesSets,
				SecondsAgoField("lastPropertiesSet", state.lastPixelShaderPropertiesSet),
				state.pixelShaderPropertiesReads,
				SecondsAgoField("lastPropertiesRead", state.lastPixelShaderPropertiesRead));
		}

		// Caller holds mtx.
		std::string ApiSection()
		{
			return std::format(
				"\"api\":{{"
				"\"pixelShaderPropertiesHookDispatched\":{},"
				"{}"
				"}}",
				Bool(state.pixelShaderPropertiesHookDispatched),
				SecondsAgoField("lastDispatch", state.lastPixelShaderPropertiesHookDispatch));
		}

		// Caller holds mtx.
		std::string LocalMapSection()
		{
			std::string frame;

			if (state.haveLastFrame)
			{
				const LocalMapFrame& f = state.lastFrame;

				std::string camera;

				if (f.haveCamera)
				{
					camera = std::format(
						"\"cameraZoom\":{:.3f},"
						"\"cameraMinExtent\":{{\"x\":{:.1f},\"y\":{:.1f}}},"
						"\"cameraMaxExtent\":{{\"x\":{:.1f},\"y\":{:.1f}}}",
						f.cameraZoom, f.cameraMinExtentX, f.cameraMinExtentY, f.cameraMaxExtentX, f.cameraMaxExtentY);
				}
				else
				{
					camera = "\"cameraZoom\":null,\"cameraMinExtent\":null,\"cameraMaxExtent\":null";
				}

				frame = std::format(
					"\"markersAdded\":{},"
					"\"markersSkippedBySetting\":{},"
					"\"actorHandlesScanned\":{},"
					"\"displayRadiiFeet\":{{\"alive\":{},\"undead\":{},\"dead\":{}}},"
					"\"screenBounds\":{{\"left\":{:.1f},\"top\":{:.1f},\"right\":{:.1f},\"bottom\":{:.1f}}},"
					"{}",
					f.markersAdded, f.markersSkippedBySetting, f.actorHandlesScanned,
					f.aliveRadiusFeet, f.undeadRadiusFeet, f.deadRadiusFeet,
					f.boundsLeft, f.boundsTop, f.boundsRight, f.boundsBottom,
					camera);
			}
			else
			{
				frame =
					"\"markersAdded\":null,"
					"\"markersSkippedBySetting\":null,"
					"\"actorHandlesScanned\":null,"
					"\"displayRadiiFeet\":null,"
					"\"screenBounds\":null,"
					"\"cameraZoom\":null,"
					"\"cameraMinExtent\":null,"
					"\"cameraMaxExtent\":null";
			}

			return std::format(
				"\"localMap\":{{"
				"\"opens\":{},"
				"{},"
				"\"frames\":{},"
				"{},"
				"\"peakMarkersAdded\":{},"
				"\"lastFrame\":{{{}}},"
				"\"extraMarkerDataNotReadyFrames\":{},"
				"{},"
				"\"borderDraws\":{},"
				"{}"
				"}}",
				state.localMapOpens,
				SecondsAgoField("lastOpen", state.lastLocalMapOpen),
				state.localMapFrames,
				SecondsAgoField("lastFrame", state.lastLocalMapFrame),
				state.peakMarkersAdded,
				frame,
				state.extraMarkerDataNotReadyFrames,
				SecondsAgoField("lastExtraMarkerDataNotReady", state.lastExtraMarkerDataNotReady),
				state.mapBorderDraws,
				SecondsAgoField("lastBorderDraw", state.lastMapBorderDraw));
		}

		// Caller holds mtx.
		std::string PlayerMarkerSection()
		{
			return std::format(
				"\"playerSetMarker\":{{"
				"\"prompts\":{},"
				"{},"
				"\"placed\":{},"
				"{},"
				"\"removed\":{},"
				"{},"
				"\"placeFailures\":{},"
				"\"lastPlaceFailureReason\":\"{}\","
				"{}"
				"}}",
				state.playerMarkerPrompts,
				SecondsAgoField("lastPrompt", state.lastPlayerMarkerPrompt),
				state.playerMarkersPlaced,
				SecondsAgoField("lastPlaced", state.lastPlayerMarkerPlaced),
				state.playerMarkersRemoved,
				SecondsAgoField("lastRemoved", state.lastPlayerMarkerRemoved),
				state.playerMarkerPlaceFailures,
				EscapeJson(state.lastPlayerMarkerPlaceFailureReason),
				SecondsAgoField("lastPlaceFailure", state.lastPlayerMarkerPlaceFailure));
		}

		// Reads "op" and "value" out of the args JSON without pulling in a parser: the strings are
		// this tool's own, and a driving tool that needed a JSON library to answer one question
		// would be worse than the question.
		std::string ArgString(const std::string& a_args, const char* a_key)
		{
			const std::string needle = std::string("\"") + a_key + "\"";
			const auto at = a_args.find(needle);
			if (at == std::string::npos) { return {}; }
			const auto colon = a_args.find(':', at + needle.size());
			if (colon == std::string::npos) { return {}; }
			auto start = a_args.find_first_not_of(" \t", colon + 1);
			if (start == std::string::npos) { return {}; }
			if (a_args[start] == '"')
			{
				const auto end = a_args.find('"', start + 1);
				return end == std::string::npos ? std::string{} : a_args.substr(start + 1, end - start - 1);
			}
			const auto end = a_args.find_first_of(",}", start);
			return a_args.substr(start, (end == std::string::npos ? a_args.size() : end) - start);
		}

		void StatusTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			// op=borderstyle sets the local map's border style live (0 = knotwork, 1 = plain), which
			// is what lets both styles be photographed in one game session.
			const std::string args = a_argsJson ? a_argsJson : "{}";
			if (ArgString(args, "op") == "borderstyle")
			{
				const std::string value = ArgString(args, "value");
				const std::uint32_t style = (value == "1") ? 1u : 0u;
				settings::mapmenu::localMapBorderStyle = style;
				logger::info("DevBench: local map border style -> {}",
							 style == 0 ? "skyrim (art)" : "untarnished");
				const std::string reply = std::string("{\"ok\":true,\"op\":\"borderstyle\",\"value\":") +
										  std::to_string(style) + "}";
				a_write(a_sink, reply.c_str());
				return;
			}

			std::string json;

			{
				std::scoped_lock lock(mtx);

				json = "{";
				json += SettingsSection();
				json += ',';
				json += StartupSection();
				json += ',';
				json += HooksSection();
				json += ',';
				json += ShaderSection();
				json += ',';
				json += ApiSection();
				json += ',';
				json += LocalMapSection();
				json += ',';
				json += PlayerMarkerSection();
				json += '}';
			}

			a_write(a_sink, json.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;

		if (registered)
		{
			return;
		}

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();

		if (!devBench)
		{
			if (a_lastAttempt)
			{
				logger::info("DevBench not detected; skipping the \"localmapupgrade.status\" live-diagnostics tool "
							 "(logging alone still covers this session - see CLAUDE.md rule 31)");
			}
			else
			{
				// Not terminal - devbench's own server can still be finishing startup this soon
				// after kPostLoad (confirmed from a real launch's timestamps: devbench finished
				// ~100ms after kPostLoad fired, which was enough to lose the race). Retried again
				// at the next message point per rule 17.
				logger::debug("DevBench not detected yet; will retry at the next message");
			}

			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Live Local Map Upgrade state: current settings, which hooks "
			"installed, whether the custom local-map pixel shader compiled, whether the "
			"kPixelShaderPropertiesHook API message reached consumers such as Dragon's Eye "
			"Minimap, and what the last local-map frame drew (markers, screen bounds, camera "
			"zoom) plus player-set-marker counters. op=borderstyle with value 0 (knotwork) or 1 "
			"(untarnished) switches the map border's style live.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{"
			"\"op\":{\"type\":\"string\"},\"value\":{\"type\":\"string\"}}},"
			"\"readOnly\":false"
			"}";

		if (devBench->RegisterTool("localmapupgrade.status", descriptor, &StatusTool, nullptr))
		{
			logger::info("Registered \"localmapupgrade.status\" with DevBench (build {})", devBench->GetBuildNumber());
		}
		else
		{
			logger::warn("DevBench reported \"localmapupgrade.status\" replaced an existing tool of the same name");
		}

		registered = true;
	}

	void RecordHookInstalled(Hook a_hook, bool a_succeeded)
	{
		const auto index = static_cast<std::size_t>(a_hook);

		if (index >= static_cast<std::size_t>(Hook::kTotal))
		{
			return;
		}

		std::scoped_lock lock(mtx);

		state.hookAttempted[index] = true;
		state.hookInstalled[index] = a_succeeded;
	}

	void RecordInfinityUIListenerRegistered(bool a_succeeded)
	{
		std::scoped_lock lock(mtx);

		state.infinityUIListenerAttempted = true;
		state.infinityUIListenerRegistered = a_succeeded;
	}

	void RecordIconDisplayExtensionPatched()
	{
		std::scoped_lock lock(mtx);

		state.iconDisplayExtensionPatched = true;
	}

	void RecordShaderManagerInitialized()
	{
		std::scoped_lock lock(mtx);

		state.shaderManagerInitialized = true;
	}

	void RecordExtraMarkersManagerInitialized()
	{
		std::scoped_lock lock(mtx);

		state.extraMarkersManagerInitialized = true;
	}

	void RecordSettingsMenuRegistered(bool a_registered)
	{
		std::scoped_lock lock(mtx);

		state.settingsMenuAttempted = true;
		state.settingsMenuRegistered = a_registered;
	}

	void RecordLocalMapShaderFound(bool a_found)
	{
		std::scoped_lock lock(mtx);

		state.localMapShaderChecked = true;
		state.localMapShaderFound = a_found;

		if (!a_found)
		{
			state.lastShaderFailureReason = "ImageSpaceManager's ISLocalMap effect is not a BSImagespaceShader";
		}
	}

	void RecordPixelShaderVariant(bool a_succeeded, std::string_view a_failureReason)
	{
		std::scoped_lock lock(mtx);

		if (a_succeeded)
		{
			++state.shaderVariantsCreated;
		}
		else
		{
			++state.shaderVariantsFailed;
			state.lastShaderFailureReason = a_failureReason;
		}
	}

	void RecordPixelShaderPropertiesSet(bool a_shapeRound, bool a_styleColor, bool a_fogOfWar)
	{
		std::scoped_lock lock(mtx);

		state.havePixelShaderProperties = true;
		state.shapeRound = a_shapeRound;
		state.styleColor = a_styleColor;
		state.fogOfWarEnabled = a_fogOfWar;
		++state.pixelShaderPropertiesSets;
		state.lastPixelShaderPropertiesSet = clock::now();
	}

	void RecordPixelShaderPropertiesRead()
	{
		std::scoped_lock lock(mtx);

		++state.pixelShaderPropertiesReads;
		state.lastPixelShaderPropertiesRead = clock::now();
	}

	void RecordPixelShaderPropertiesHookDispatched()
	{
		std::scoped_lock lock(mtx);

		state.pixelShaderPropertiesHookDispatched = true;
		state.lastPixelShaderPropertiesHookDispatch = clock::now();
	}

	void RecordLocalMapFrame(const LocalMapFrame& a_frame)
	{
		const clock::time_point now = clock::now();

		std::scoped_lock lock(mtx);

		// AddExtraMarkers runs every frame the map is open, so a gap longer than a second means
		// the map was closed and reopened rather than that one frame was slow. That turns a
		// stream of frames into the thing a person actually asks for - "when did you last open
		// the local map" - without needing a menu-open event sink just for the diagnostics.
		if (!state.lastLocalMapFrame || (now - *state.lastLocalMapFrame) > std::chrono::seconds(1))
		{
			++state.localMapOpens;
			state.lastLocalMapOpen = now;
		}

		++state.localMapFrames;
		state.lastLocalMapFrame = now;

		state.haveLastFrame = true;
		state.lastFrame = a_frame;

		if (a_frame.markersAdded > state.peakMarkersAdded)
		{
			state.peakMarkersAdded = a_frame.markersAdded;
		}
	}

	void RecordExtraMarkerDataNotReady()
	{
		std::scoped_lock lock(mtx);

		++state.extraMarkerDataNotReadyFrames;
		state.lastExtraMarkerDataNotReady = clock::now();
	}

	void RecordMapBorderDrawn()
	{
		std::scoped_lock lock(mtx);

		++state.mapBorderDraws;
		state.lastMapBorderDraw = clock::now();
	}

	void RecordPlayerMarkerPrompt()
	{
		std::scoped_lock lock(mtx);

		++state.playerMarkerPrompts;
		state.lastPlayerMarkerPrompt = clock::now();
	}

	void RecordPlayerMarkerPlaced()
	{
		std::scoped_lock lock(mtx);

		++state.playerMarkersPlaced;
		state.lastPlayerMarkerPlaced = clock::now();
	}

	void RecordPlayerMarkerRemoved()
	{
		std::scoped_lock lock(mtx);

		++state.playerMarkersRemoved;
		state.lastPlayerMarkerRemoved = clock::now();
	}

	void RecordPlayerMarkerPlaceFailed(std::string_view a_reason)
	{
		std::scoped_lock lock(mtx);

		++state.playerMarkerPlaceFailures;
		state.lastPlayerMarkerPlaceFailureReason = a_reason;
		state.lastPlayerMarkerPlaceFailure = clock::now();
	}
}
