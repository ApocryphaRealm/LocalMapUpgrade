#pragma once

// Backs the "localmapupgrade.status" DevBench tool - see CLAUDE.md rule 31 (every mod's first
// version ships with live-queryable state, not just logs reconstructed after the fact).
//
// Every Record* function here is called from the main thread, at the exact point a decision is
// made, and only ever writes a mutex-guarded snapshot. The DevBench tool handler runs on
// devbench's own listener thread and only ever reads that snapshot - it never reaches back into
// game state itself. That matters more here than in most mods: almost everything this plugin
// touches (the local map's culling process, its Scaleform icon display, the D3D11 pixel shader)
// is only valid on the render/main thread, so a handler that "just peeked" at the LocalMapMenu
// would be a race, not a query.
namespace diagnostics
{
	// Looks up the DevBench interface (present only if the DevBench plugin is installed) and
	// registers "localmapupgrade.status". Safe to call repeatedly - a real launch showed
	// devbench's own server can still be finishing startup a moment after SKSE sends kPostLoad
	// (its own documented earliest-safe point), so this is a rule-17 retry, not a one-shot
	// lookup: call it again at kPostPostLoad and kDataLoaded too. Every call after the first
	// successful one is a cheap no-op; only the final call (a_lastAttempt = true) logs that
	// DevBench was never found, so the "not installed" conclusion isn't reported before every
	// retry is exhausted.
	void Init(bool a_lastAttempt = false);

	// --- Hook installation -------------------------------------------------------------------
	//
	// hooks::Install() runs inside SKSEPluginLoad, before any SKSE message. A hook that silently
	// failed to install looks exactly like "the feature does nothing" from in game, which is the
	// single most expensive thing to diagnose from a log after the fact - so each one records
	// whether it actually took.
	enum class Hook
	{
		kIsSmallWorld,                 // makes interior/small worldspaces render the full local map
		kAddQuestMarkersToMap,         // injects this mod's extra actor markers beside quest markers
		kInvokeCreateMarkers,          // post-processes the Scaleform "CreateMarkers" call
		kInputHandlerCanProcess,       // lets the local map accept button input it normally drops
		kInputHandlerProcessButton,    // click-to-place-marker, keyboard panning
		kWaterShaderSetupTechnique,    // fixes water flickering on the local map
		kDetectLifeEffectUpdate,       // grows the actor display radius from a Detect Life effect
		kScriptEffectUpdate,           // same, for Aura Whisper
		kStopHitEffectsVisit,          // watches for detect effects being torn down
		kShaderReferenceEffectDetach,  // collapses the display radius when a detect effect ends
		kToggleFogOfWarCommand,        // repoints the vanilla "tfow" console command

		kTotal
	};

	void RecordHookInstalled(Hook a_hook, bool a_succeeded);

	// --- Startup milestones ------------------------------------------------------------------

	// SKSE accepted (or rejected) the "InfinityUI" listener registration at kPostLoad. Without
	// Infinity UI the icon-display extension never loads and no extra marker can ever be drawn.
	void RecordInfinityUIListenerRegistered(bool a_succeeded);

	// Infinity UI reported that IconDisplayExtension.swf was patched into the local map movie.
	void RecordIconDisplayExtensionPatched();

	// ShaderManager::InitSingleton() / ExtraMarkersManager::InitSingleton() completed (kDataLoaded).
	void RecordShaderManagerInitialized();
	void RecordExtraMarkersManagerInitialized();

	// UI::Register() finished; a_registered is false when SKSE Menu Framework is absent, in which
	// case the mod still works but has no settings page.
	void RecordSettingsMenuRegistered(bool a_registered);

	// --- Shader state ------------------------------------------------------------------------

	// The skyrim_cast to BSImagespaceShader for ImageSpaceManager's ISLocalMap effect. False here
	// means NOTHING this mod does to the map's appearance can work, and it is the one failure the
	// original code only reported as a single "critical" line at startup.
	void RecordLocalMapShaderFound(bool a_found);

	// One of the eight ISLocalMap.hlsl variants (squared/round x colour/B&W x fog/no-fog) finished
	// its compile + CreatePixelShader pass. a_failureReason is empty on success.
	void RecordPixelShaderVariant(bool a_succeeded, std::string_view a_failureReason);

	// ShaderManager::SetPixelShaderProperties swapped the live pixel shader. Called once per frame
	// by Dragon's Eye Minimap, so this is deliberately a counter plus two enum stores and nothing
	// else. a_shapeRound/a_styleColor mirror PixelShaderProperty::Shape/Style without dragging
	// API.h into this header.
	void RecordPixelShaderPropertiesSet(bool a_shapeRound, bool a_styleColor, bool a_fogOfWar);

	// ShaderManager::GetPixelShaderProperties was called - i.e. a CONSUMER of this mod's API
	// (Dragon's Eye Minimap) is alive and polling. Distinguishing "nobody is asking" from "asking
	// and getting the wrong answer" is most of the work when the two mods disagree about style.
	void RecordPixelShaderPropertiesRead();

	// The kPixelShaderPropertiesHook API message went out at kDataLoaded. Consumers only get the
	// two function pointers through this one dispatch, so "did it fire" is the first question to
	// ask when a consumer behaves as if this mod is not installed.
	void RecordPixelShaderPropertiesHookDispatched();

	// --- Local map activity ------------------------------------------------------------------

	// One frame of ExtraMarkersManager::AddExtraMarkers. Everything here was already computed by
	// that function for its own change-only logging, so this adds one lock and some stores per
	// frame the local map is open - and only while it is open.
	struct LocalMapFrame
	{
		std::uint32_t markersAdded = 0;
		std::uint32_t markersSkippedBySetting = 0;
		std::size_t actorHandlesScanned = 0;
		std::uint32_t aliveRadiusFeet = 0;
		std::uint32_t undeadRadiusFeet = 0;
		std::uint32_t deadRadiusFeet = 0;
		float boundsLeft = 0.0F;
		float boundsTop = 0.0F;
		float boundsRight = 0.0F;
		float boundsBottom = 0.0F;
		bool haveCamera = false;
		float cameraZoom = 0.0F;
		float cameraMinExtentX = 0.0F;
		float cameraMinExtentY = 0.0F;
		float cameraMaxExtentX = 0.0F;
		float cameraMaxExtentY = 0.0F;
	};

	void RecordLocalMapFrame(const LocalMapFrame& a_frame);

	// AddExtraMarkers found the Scaleform ExtraMarkerData array missing - the icon-display
	// extension is not ready (or not installed), so no extra marker can be drawn this frame.
	void RecordExtraMarkerDataNotReady();

	// DrawMapBorder actually stroked the border rectangle (bLocalMapBorder on and the map's own
	// rectangle usable).
	void RecordMapBorderDrawn();

	// --- Player-set marker -------------------------------------------------------------------

	// The move/leave/remove message box was opened over an existing player marker.
	void RecordPlayerMarkerPrompt();

	// A custom destination marker was actually written to the player.
	void RecordPlayerMarkerPlaced();

	// The player's custom destination marker was removed.
	void RecordPlayerMarkerRemoved();

	// A place attempt got as far as the ray cast and gave up - a_reason names where.
	void RecordPlayerMarkerPlaceFailed(std::string_view a_reason);
}
