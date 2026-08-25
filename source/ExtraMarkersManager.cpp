#include "ExtraMarkersManager.h"
#include "RE/M/MapMenu.h"

#include "Settings.h"
#include "utils/Logger.h"

#include "RE/L/LocalMapCamera.h"
#include "RE/M/MapMenuMarker.h"

namespace RE
{
	std::int32_t TESObjectREFR_GetInventoryCount(TESObjectREFR* a_object, bool a_useDataHandlerInventory = false, bool a_unk03 = false)
	{
		using func_t = decltype(&TESObjectREFR_GetInventoryCount);
		REL::Relocation<func_t> func{ REL::VariantID{ 19274, 19700, 0x29F980 } };
		return func(a_object, a_useDataHandlerInventory, a_unk03);
	}

	std::int32_t ExtraDataList_GetDroppedWeapon(ExtraDataList* a_extraList, TESObjectREFRPtr& a_weapon)
	{
		using func_t = decltype(&ExtraDataList_GetDroppedWeapon);
		REL::Relocation<func_t> func{ REL::VariantID{ 11616, 11762, 0x1266A0 } };
		return func(a_extraList, a_weapon);
	}

	std::int32_t ExtraDataList_GetDroppedUtil(ExtraDataList* a_extraList, TESObjectREFRPtr& a_util)
	{
		using func_t = decltype(&ExtraDataList_GetDroppedUtil);
		REL::Relocation<func_t> func{ REL::VariantID{ 11617, 11763, 0x126870 } };
		return func(a_extraList, a_util);
	}

	bool TESObjectREFR_HasAnyDroppedItem(TESObjectREFR* a_ref)
	{
		if (std::int32_t inventoryCount = RE::TESObjectREFR_GetInventoryCount(a_ref))
		{
			return true;
		}
		else
		{
			if (a_ref->formType == RE::FormType::ActorCharacter)
			{
				RE::TESObjectREFRPtr carriedDroppedWeapon;
				RE::ExtraDataList_GetDroppedWeapon(&a_ref->extraList, carriedDroppedWeapon);
				if (carriedDroppedWeapon)
				{
					return true;
				}

				RE::TESObjectREFRPtr carriedDroppedUtil;
				RE::ExtraDataList_GetDroppedUtil(&a_ref->extraList, carriedDroppedUtil);
				if (carriedDroppedUtil)
				{
					return true;
				}
			}

			return false;
		}
	}

	// Added here because the virtual function seems broken in CommonLibVR
	bool Actor__IsDead(Actor* a_actor, bool a_notEssential = true)
	{
		ACTOR_LIFE_STATE lifeState = a_actor->AsActorState()->actorState1.lifeState;

		if (lifeState == ACTOR_LIFE_STATE::kDying ||
			lifeState == ACTOR_LIFE_STATE::kDead ||
			lifeState == ACTOR_LIFE_STATE::kRecycle)
		{
			return true;
		}

		if (!a_notEssential)
		{
			return lifeState == ACTOR_LIFE_STATE::kEssentialDown;
		}

		return false;
	}

	bool Actor__IsUndead(Actor* a_actor)
	{
		return a_actor->GetRace()->HasAnyKeywordByEditorID({ "ActorTypeDaedra", "ActorTypeDwarven", "NoDetectLife", "ActorTypeUndead" });
	}
}

namespace LMU
{
	void ExtraMarkersManager::AddExtraMarker(RE::ActorHandle& a_actorHandle, RE::Actor* a_actor,
											 RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers)
	{
		RE::MapMenuMarker mapMarker
		{
			.data = nullptr,
			.ref = a_actorHandle.native_handle(),
			.description = a_actor->GetDisplayFullName(),
			.type = RE::MapMenuMarker::Type::kLocation, // Playing mind tricks with the game
			.door = 0,
			.index = -1,
			.quest = nullptr,
			.unk30 = 1
		};

		a_mapMarkers.push_back(mapMarker);
	}

	void ExtraMarkersManager::DrawMapBorder(RE::LocalMapMenu& a_localMapMenu)
	{
		// Only draw when the real map menu is open.
		//
		// Dragon's Eye Minimap allocates its own RE::LocalMapMenu instance for the minimap, so
		// this function runs against that instance too - and the border was being drawn into the
		// minimap instead of the local map. the author saw exactly that: no border on the local map,
		// but one inside the minimap. The LocalMapHolder bounds of (0,0)-(386,386) were the
		// minimap's, not a full-screen map's.
		//
		// upstream's PlayerSetMarkerManager already guards the same way, with the comment "Only if
		// the map menu is open. Make sure because someone maybe makes a minimap mod." That advice
		// was already in this codebase and should have been followed here first time.
		if (!RE::UI::GetSingleton()->GetMenu<RE::MapMenu>().get())
		{
			return;
		}

		RE::GFxValue& root = a_localMapMenu.GetRuntimeData().root;

		if (!root.IsDisplayObject())
		{
			return;
		}

		RE::GFxValue border;
		const bool exists = root.GetMember("LMUMapBorder", &border) && border.IsDisplayObject();

		if (!settings::mapmenu::localMapBorder)
		{
			// Turned off - wipe whatever was drawn rather than leaving it on screen. Cheap, and
			// it makes the toggle apply live like every other setting in this mod.
			if (exists)
			{
				border.Invoke("clear");
			}

			return;
		}

		if (!exists)
		{
			// A high depth so the border sits above the map and its markers rather than under them.
			std::array<RE::GFxValue, 2> create{ RE::GFxValue{ "LMUMapBorder" }, RE::GFxValue{ 10000.0 } };

			if (!root.Invoke("createEmptyMovieClip", &border, create.data(), create.size()) || !border.IsDisplayObject())
			{
				static bool warnedNoClip = false;

				if (!warnedNoClip)
				{
					logger::warn("DrawMapBorder: could not create the border clip; the map border will not be drawn");
					warnedNoClip = true;
				}

				return;
			}
		}

		// Measure the clip the map is actually drawn into, rather than computing a rectangle.
		//
		// The first attempt used LocalMapMenu's own topLeft/bottomRight. Those are the map's
		// *local* extents, not menu coordinates - the author's log had them at (600,325)-(2600,1450) and
		// (2747,22)-(3229,505) against a 1280x720 stage, so the border drew far off to the
		// bottom-right. Dragon's Eye Minimap needs a whole SetLocalMapExtents conversion to turn
		// those into screen space, which is the clue that they were never the right input here.
		//
		// getBounds on the holder clip, measured against the same parent the border is drawn on,
		// needs no conversion and follows the map wherever the game puts it.
		// One-time dump of what the menu root actually contains. LocalMapHolder was a reasonable
		// guess - it is what Map.LocalMap declares - but it is not reliably reachable here, and
		// this is the second placement guess to miss. Rather than try a third, list the real
		// members once so the right clip can be chosen from evidence.
		{
			static bool dumped = false;

			if (!dumped)
			{
				dumped = true;

				// Measure every display object, not only the ones with promising names. Four
				// name-based guesses have missed, and the plate has been shown by measurement not to
				// be any of them - so the useful question is no longer "is this clip the plate" but
				// "which clip, if any, has the plate's dimensions". The plate is 838.7x492.3 at
				// aspect 1.704; a match in this list is the answer, and no match proves the plate is
				// not a direct child of the root at all.
				// Descends one level, because the plate is very likely a child rather than a root
				// member. Dragon's Eye Minimap hit exactly this: getBounds measures a clip's full
				// render extent regardless of _visible, so a container's bounds are inflated by
				// children that never draw. Its fix was to stop measuring the container and measure
				// BackgroundArtSquare/BackgroundArtCircle - the clip that is exactly the visible art.
				// Background here measures 915.9x604.6 against a visible plate of 838.7x492.3, which
				// is the same symptom, so the plate is probably a named art clip inside it.
				struct MemberDump : RE::GFxValue::ObjectVisitor
				{
					RE::GFxValue* against = nullptr;
					const char* prefix = "";
					int depth = 0;

					void Visit(const char* a_name, const RE::GFxValue& a_val) override
					{
						const char* kind = a_val.IsDisplayObject() ? "displayObject" :
										   a_val.IsArray() ? "array" :
										   a_val.IsObject() ? "object" :
										   a_val.IsString() ? "string" :
										   a_val.IsNumber() ? "number" :
										   a_val.IsBool() ? "bool" : "other";

						if (!a_val.IsDisplayObject() || !against)
						{
							logger::info("  root member: {} ({})", a_name, kind);

							return;
						}

						RE::GFxValue clip = a_val;
						RE::GFxValue bounds;
						std::array<RE::GFxValue, 1> args{ *against };

						if (!clip.Invoke("getBounds", &bounds, args.data(), args.size()) || !bounds.IsObject())
						{
							logger::info("  root member: {} (displayObject, getBounds failed)", a_name);

							return;
						}

						RE::GFxValue xMin, xMax, yMin, yMax;

						if (!bounds.GetMember("xMin", &xMin) || !bounds.GetMember("xMax", &xMax) ||
							!bounds.GetMember("yMin", &yMin) || !bounds.GetMember("yMax", &yMax))
						{
							logger::info("  root member: {} (displayObject, bounds unreadable)", a_name);

							return;
						}

						const double l = xMin.GetNumber();
						const double t = yMin.GetNumber();
						const double r = xMax.GetNumber();
						const double b = yMax.GetNumber();
						const double w = r - l;
						const double h = b - t;

						// The plate, measured from a screenshot. Flag anything close so the match is
						// obvious in the log rather than something to spot by eye among twenty lines.
						const bool matchesPlate = std::abs(w - 838.7) < 25.0 && std::abs(h - 492.3) < 25.0;

						logger::info("  {}{} ({},{})-({},{}) {}x{} aspect {:.3f}{}",
							prefix, a_name, l, t, r, b, w, h, h != 0.0 ? w / h : 0.0,
							matchesPlate ? "   <<<< MATCHES THE PLATE" : "");

						// One level down is enough. The art clip sits directly inside its container in
						// every case seen so far, and recursing without a bound risks walking the whole
						// display list into the marker clips.
						if (depth >= 1)
						{
							return;
						}

						std::string childPrefix = std::string("    ") + a_name + ".";

						MemberDump child;
						child.against = against;
						child.prefix = childPrefix.c_str();
						child.depth = depth + 1;
						clip.VisitMembers(&child);
					}
				};

				logger::info("DrawMapBorder: measuring every display object on the map root, one level deep -");
				logger::info("  (looking for 838.7x492.3, aspect 1.704 - the plate measured from a screenshot)");
				MemberDump visitor;
				visitor.against = &root;
				visitor.prefix = "root member: ";
				root.VisitMembers(&visitor);
			}
		}

		// Names taken from the root member enumeration, not guessed. The first attempt looked for
		// "LocalMapHolder" because that is what Map.LocalMap declares; the real member is
		// "LocalMapHolder_mc".
		//
		// NONE of these is the black plate. Measured from a clean screenshot - scanning inward from
		// the parchment on each side, so nothing drawn inside the plate can truncate the scan - the
		// plate is (-22.8,-22.3)-(816.0,469.9), 838.7x492.3, aspect 1.704. LocalMapHolder_mc and
		// LocalMapRect are both ~1.778 and sit inside it; Background is 1.515 and falls outside it.
		// The plate lies between them and matches nothing on this list.
		//
		// An earlier measurement claimed the plate was (0,0)-(800,450) at aspect 1.784. That was
		// wrong: it found the longest black run on each scan line while the test build's own overlay
		// rectangles were drawn across the plate, so it measured the gap between two of those
		// rectangles rather than the plate itself.
		//
		// So this list is now only a fallback. The real target is found by measuring every display
		// object on the root - see the sweep below.
		constexpr const char* kRectCandidates[] = {
			"LocalMapHolder_mc",
			"TextureHolder",
			"LocalMapRect",
			"Background",
		};

		RE::GFxValue holder;
		const char* holderName = nullptr;

		// Measure and log every candidate once, not only the one that wins. Choosing the right clip
		// has been the entire difficulty with this feature and has cost several builds, so if this
		// choice is also wrong the next one can be made from the log rather than from another guess.
		static bool measuredCandidates = false;

		if (!measuredCandidates)
		{
			measuredCandidates = true;

			for (const char* name : kRectCandidates)
			{
				RE::GFxValue candidate;

				if (!root.GetMember(name, &candidate) || !candidate.IsDisplayObject())
				{
					logger::debug("DrawMapBorder: candidate {} is not present on the map root", name);

					continue;
				}

				RE::GFxValue candidateBounds;
				std::array<RE::GFxValue, 1> candidateAgainst{ root };

				if (!candidate.Invoke("getBounds", &candidateBounds, candidateAgainst.data(), candidateAgainst.size()) ||
					!candidateBounds.IsObject())
				{
					logger::debug("DrawMapBorder: candidate {} is present but getBounds failed", name);

					continue;
				}

				RE::GFxValue cxMin, cxMax, cyMin, cyMax;

				if (candidateBounds.GetMember("xMin", &cxMin) && candidateBounds.GetMember("xMax", &cxMax) &&
					candidateBounds.GetMember("yMin", &cyMin) && candidateBounds.GetMember("yMax", &cyMax))
				{
					const double left = cxMin.GetNumber();
					const double right = cxMax.GetNumber();
					const double top = cyMin.GetNumber();
					const double bottom = cyMax.GetNumber();
					const double width = right - left;
					const double height = bottom - top;

					logger::debug("DrawMapBorder: candidate {} bounds ({},{}) to ({},{}) - {}x{}, aspect {:.3f}",
						name, left, top, right, bottom, width, height,
						height != 0.0 ? width / height : 0.0);
				}
			}
		}

		// Ask the game where the map actually is, rather than guessing which clip represents it.
		//
		// Four builds were spent picking clips, and the measurements from 1.1.6 show why none of
		// them can be right: LocalMapRect, LocalMapHolder_mc and TextureHolder are all the 800x450
		// stage (aspect 1.778) and sit inside the black plate, while Background is 915.9x604.6
		// (aspect 1.515) and falls well outside it. The plate is between the two, so no clip in the
		// list describes it.
		//
		// LocalMapMenu carries topLeft/bottomRight, which the game maintains as the map's extents.
		// 1.1.0 read exactly those and put the border off toward the bottom-right corner, which is
		// how they got dismissed - but the fault was not the numbers, it was using them raw. They
		// are in the map's own local space (the author's log had (600,325)-(2600,1450)), not menu
		// coordinates. Dragon's Eye Minimap converts the same pair with TranslateToScreen and an
		// identity matrix, and that conversion is what 1.1.0 was missing.
		// DISABLED in the border test build. The extents give the correct aspect (1.778) but
		// TranslateToScreen returns real screen pixels, while this clip draws in the menu's
		// 800x450 stage space - it produced a 5000x2812.5 rectangle drawn far off-screen.
		// Kept for its logging, which is what established the map is 16:9.
		constexpr bool kUseExtentsForPlacement = false;
		bool haveExtents = false;
		float left = 0.0F, top = 0.0F, right = 0.0F, bottom = 0.0F;

		if (auto* movieView = a_localMapMenu.GetRuntimeData().movieView)
		{
			const RE::GPointF localTopLeft = a_localMapMenu.topLeft;
			const RE::GPointF localBottomRight = a_localMapMenu.bottomRight;

			// An unpopulated pair means the map has not reported its extents yet - it arrives a few
			// frames after the menu opens. Fall through to the clip measurement rather than drawing a
			// zero-sized border, and pick the extents up on a later frame (rule 17).
			if (localBottomRight.x - localTopLeft.x > 1.0F && localBottomRight.y - localTopLeft.y > 1.0F)
			{
				float identityMat2D[2][3] = { { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };

				const RE::GPointF screenTopLeft = movieView->TranslateToScreen(localTopLeft, identityMat2D);
				const RE::GPointF screenBottomRight = movieView->TranslateToScreen(localBottomRight, identityMat2D);

				left = screenTopLeft.x;
				top = screenTopLeft.y;
				right = screenBottomRight.x;
				bottom = screenBottomRight.y;
				haveExtents = kUseExtentsForPlacement;

				static bool loggedExtents = false;

				if (!loggedExtents)
				{
					loggedExtents = true;

					const float width = right - left;
					const float height = bottom - top;

					logger::info("DrawMapBorder: map extents local ({},{})-({},{}) -> screen ({},{})-({},{}) - {}x{}, aspect {:.3f}",
						localTopLeft.x, localTopLeft.y, localBottomRight.x, localBottomRight.y,
						left, top, right, bottom, width, height,
						height != 0.0F ? width / height : 0.0F);
				}
			}
		}

		if (!haveExtents)
		{
			for (const char* name : kRectCandidates)
			{
				if (root.GetMember(name, &holder) && holder.IsDisplayObject())
				{
					holderName = name;

					break;
				}
			}
		}

		if (!haveExtents)
		{
			if (!holderName)
			{
				static bool warnedNoHolder = false;

				if (!warnedNoHolder)
				{
					logger::warn("DrawMapBorder: no map extents yet and none of the expected map clips are present; the border cannot be placed");
					warnedNoHolder = true;
				}

				border.Invoke("clear");

				return;
			}

			RE::GFxValue bounds;
			std::array<RE::GFxValue, 1> against{ root };

			if (!holder.Invoke("getBounds", &bounds, against.data(), against.size()) || !bounds.IsObject())
			{
				static bool warnedNoBounds = false;

				if (!warnedNoBounds)
				{
					logger::warn("DrawMapBorder: getBounds on {} failed; the border cannot be placed", holderName);
					warnedNoBounds = true;
				}

				return;
			}

			RE::GFxValue xMin, xMax, yMin, yMax;
			bounds.GetMember("xMin", &xMin);
			bounds.GetMember("xMax", &xMax);
			bounds.GetMember("yMin", &yMin);
			bounds.GetMember("yMax", &yMax);

			left = static_cast<float>(xMin.GetNumber());
			top = static_cast<float>(yMin.GetNumber());
			right = static_cast<float>(xMax.GetNumber());
			bottom = static_cast<float>(yMax.GetNumber());
		}

		// Runs every frame the map is open, so log only when the rectangle actually moves
		// (CLAUDE.md rule 14). These are the numbers to check first if the border lands wrong.
		{
			static float lastLeft = 0.0F, lastTop = 0.0F, lastRight = 0.0F, lastBottom = 0.0F;

			if (left != lastLeft || top != lastTop || right != lastRight || bottom != lastBottom)
			{
				lastLeft = left; lastTop = top; lastRight = right; lastBottom = bottom;
				logger::debug("DrawMapBorder: placed from {} - ({},{}) to ({},{})",
					haveExtents ? "map extents" : (holderName ? holderName : "unknown"), left, top, right, bottom);
			}
		}
		constexpr double kBorderThickness = 2.0;
		constexpr double kBorderAlpha = 100.0;

		border.Invoke("clear");

		auto draw = [&](const char* a_call, float a_x, float a_y) {
			std::array<RE::GFxValue, 2> point{ RE::GFxValue{ static_cast<double>(a_x) }, RE::GFxValue{ static_cast<double>(a_y) } };
			(void)border.Invoke(a_call, nullptr, point.data(), point.size());
		};

		auto strokeRect = [&](double a_colour, float a_left, float a_top, float a_right, float a_bottom) {
			std::array<RE::GFxValue, 3> style{ RE::GFxValue{ kBorderThickness }, RE::GFxValue{ a_colour },
											   RE::GFxValue{ kBorderAlpha } };
			(void)border.Invoke("lineStyle", nullptr, style.data(), style.size());

			draw("moveTo", a_left, a_top);
			draw("lineTo", a_right, a_top);
			draw("lineTo", a_right, a_bottom);
			draw("lineTo", a_left, a_bottom);
			draw("lineTo", a_left, a_top);
		};

		// Untarnished UI's off-white, the same colour the frame reskin uses.
		constexpr double kBorderColour = 0xF5F2E9;

		strokeRect(kBorderColour, left, top, right, bottom);
	}

	void ExtraMarkersManager::AddExtraMarkers(RE::LocalMapMenu& a_localMapMenu)
	{
		// Runs every frame the local map is open. CLAUDE.md rule 14's one hard constraint is that
		// nothing here logs unconditionally - and since rule 14 now also has every mod shipping at
		// trace, the log level will not save us either. Log the configuration only when it changes.
		{
			const auto state = std::make_tuple(
				settings::mapmenu::localMapShowEnemyActors, settings::mapmenu::localMapShowHostileActors,
				settings::mapmenu::localMapShowGuardActors, settings::mapmenu::localMapShowDeadActors,
				settings::mapmenu::localMapShowTeammateActors, settings::mapmenu::localMapShowNeutralActors,
				settings::mapmenu::localMapShowActorsOnlyWithDetectSpell,
				GetAliveActorsDisplayRadius(), GetUndeadActorsDisplayRadius(), GetDeadActorsDisplayRadius());

			static std::optional<std::remove_const_t<decltype(state)>> lastLogged;

			if (!lastLogged || *lastLogged != state)
			{
				lastLogged = state;

				logger::debug("AddExtraMarkers: enemy={} hostile={} guard={} dead={} teammate={} neutral={} immersive={} "
							  "(radii ft - alive={} undead={} dead={})",
							  std::get<0>(state), std::get<1>(state), std::get<2>(state), std::get<3>(state),
							  std::get<4>(state), std::get<5>(state), std::get<6>(state),
							  std::get<7>(state), std::get<8>(state), std::get<9>(state));
			}
		}

		DrawMapBorder(a_localMapMenu);

		RE::GFxValue extraMarkersData;
		a_localMapMenu.GetRuntimeData().iconDisplay.GetMember("ExtraMarkerData", &extraMarkersData);

		if (!extraMarkersData.IsArray())
		{
			logger::debug("AddExtraMarkers: ExtraMarkerData is not an array (extension not ready?) - no markers added");

			return;
		}

		extraMarkersData.ClearElements();

		RE::BSTArray<RE::MapMenuMarker>& mapMarkers = a_localMapMenu.mapMarkers;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::BSTArray<RE::ActorHandle>& actorHandles = RE::ProcessLists::GetSingleton()->highActorHandles;

		static std::size_t lastLoggedHandleCount = static_cast<std::size_t>(-1);
		if (actorHandles.size() != lastLoggedHandleCount)
		{
			lastLoggedHandleCount = actorHandles.size();
			logger::debug("AddExtraMarkers: scanning {} nearby actor handle(s)", actorHandles.size());
		}

		RE::BSTArray<RE::ActorHandle>& enemyHandles = REL::Module::IsVR() ? player->GetVRInfoRuntimeData()->actorsToDisplayOnTheHUDArray :
																			player->GetInfoRuntimeData().actorsToDisplayOnTheHUDArray;

		std::uint32_t addedCount = 0;
		std::uint32_t skippedBySettingCount = 0;

		auto addMarker = [&](RE::ActorHandle& a_handle, RE::Actor* a_actor, ExtraMarker::Type a_type) {
			AddExtraMarker(a_handle, a_actor, mapMarkers);
			extraMarkersData.PushBack(a_type);
			++addedCount;
		};

		for (RE::ActorHandle& actorHandle : actorHandles)
		{
			if (RE::Actor* actor = actorHandle.get().get())
			{
				float distance = actor->GetDistance(player);

				bool isDead = Actor__IsDead(actor);

				if (isDead)
				{
					if (distance <= deadActorsDisplayRadius)
					{
						if (RE::TESObjectREFR_HasAnyDroppedItem(actor))
						{
							if (settings::mapmenu::localMapShowDeadActors)
							{
								addMarker(actorHandle, actor, ExtraMarker::Type::kDead);
							}
							else
							{
								++skippedBySettingCount;
							}
						}
					}
				}
				else
				{
					bool isUndead = Actor__IsUndead(actor);
					bool isAlive = !isUndead;

					if ((isAlive && distance <= aliveActorsDisplayRadius) ||
						(isUndead && distance <= undeadActorsDisplayRadius))
					{
						bool isEnemy = false;

						for (RE::ActorHandle& enemyActorHandle : enemyHandles)
						{
							if (actorHandle == enemyActorHandle)
							{
								isEnemy = true;
								break;
							}
						}

						if (isEnemy)
						{
							if (settings::mapmenu::localMapShowEnemyActors)
							{
								addMarker(actorHandle, actor, ExtraMarker::Type::kEnemy);
							}
							else
							{
								++skippedBySettingCount;
							}
						}
						else
						{
							if (actor->IsPlayerTeammate())
							{
								if (settings::mapmenu::localMapShowTeammateActors)
								{
									addMarker(actorHandle, actor, ExtraMarker::Type::kTeammate);
								}
								else
								{
									++skippedBySettingCount;
								}
							}
							else if (actor->IsHostileToActor(player))
							{
								if (settings::mapmenu::localMapShowHostileActors)
								{
									addMarker(actorHandle, actor, ExtraMarker::Type::kHostile);
								}
								else
								{
									++skippedBySettingCount;
								}
							}
							else if (actor->IsGuard())
							{
								if (settings::mapmenu::localMapShowGuardActors)
								{
									addMarker(actorHandle, actor, ExtraMarker::Type::kGuard);
								}
								else
								{
									++skippedBySettingCount;
								}
							}
							else
							{
								if (settings::mapmenu::localMapShowNeutralActors)
								{
									addMarker(actorHandle, actor, ExtraMarker::Type::kNeutral);
								}
								else
								{
									++skippedBySettingCount;
								}
							}
						}
					}
				}
			}
		}

		static std::uint32_t lastLoggedAdded = static_cast<std::uint32_t>(-1);
		static std::uint32_t lastLoggedSkipped = static_cast<std::uint32_t>(-1);
		if (addedCount != lastLoggedAdded || skippedBySettingCount != lastLoggedSkipped)
		{
			lastLoggedAdded = addedCount;
			lastLoggedSkipped = skippedBySettingCount;
			logger::debug("AddExtraMarkers: added {} marker(s), {} skipped by a visibility setting", addedCount, skippedBySettingCount);
		}
	}

	void ExtraMarkersManager::PostCreateMarkers(RE::GFxValue& a_iconDisplay)
	{
		a_iconDisplay.Invoke("PostCreateMarkers");
	}
}