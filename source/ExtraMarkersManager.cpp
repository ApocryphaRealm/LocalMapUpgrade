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

				struct MemberDump : RE::GFxValue::ObjectVisitor
				{
					void Visit(const char* a_name, const RE::GFxValue& a_val) override
					{
						const char* kind = a_val.IsDisplayObject() ? "displayObject" :
										   a_val.IsArray() ? "array" :
										   a_val.IsObject() ? "object" :
										   a_val.IsString() ? "string" :
										   a_val.IsNumber() ? "number" :
										   a_val.IsBool() ? "bool" : "other";

						logger::info("  root member: {} ({})", a_name, kind);
					}
				};

				logger::info("DrawMapBorder: enumerating the local map menu root's members -");
				MemberDump visitor;
				root.VisitMembers(&visitor);
			}
		}

		RE::GFxValue holder;

		if (!root.GetMember("LocalMapHolder", &holder) || !holder.IsDisplayObject())
		{
			static bool warnedNoHolder = false;

			if (!warnedNoHolder)
			{
				logger::warn("DrawMapBorder: could not reach LocalMapHolder; the border cannot be placed");
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
				logger::warn("DrawMapBorder: getBounds on LocalMapHolder failed; the border cannot be placed");
				warnedNoBounds = true;
			}

			return;
		}

		RE::GFxValue xMin, xMax, yMin, yMax;
		bounds.GetMember("xMin", &xMin);
		bounds.GetMember("xMax", &xMax);
		bounds.GetMember("yMin", &yMin);
		bounds.GetMember("yMax", &yMax);

		const float left = static_cast<float>(xMin.GetNumber());
		const float top = static_cast<float>(yMin.GetNumber());
		const float right = static_cast<float>(xMax.GetNumber());
		const float bottom = static_cast<float>(yMax.GetNumber());

		// Runs every frame the map is open, so log only when the rectangle actually moves
		// (CLAUDE.md rule 14). These are the numbers to check first if the border lands wrong.
		{
			static float lastLeft = 0.0F, lastTop = 0.0F, lastRight = 0.0F, lastBottom = 0.0F;

			if (left != lastLeft || top != lastTop || right != lastRight || bottom != lastBottom)
			{
				lastLeft = left; lastTop = top; lastRight = right; lastBottom = bottom;
				logger::debug("DrawMapBorder: LocalMapHolder bounds ({},{}) to ({},{})", left, top, right, bottom);
			}
		}
		// Untarnished UI's off-white, the same colour the frame reskin uses.
		constexpr double kBorderColour = 0xF5F2E9;
		constexpr double kBorderThickness = 2.0;
		constexpr double kBorderAlpha = 100.0;

		border.Invoke("clear");

		std::array<RE::GFxValue, 3> style{ RE::GFxValue{ kBorderThickness }, RE::GFxValue{ kBorderColour },
										   RE::GFxValue{ kBorderAlpha } };
		(void)border.Invoke("lineStyle", nullptr, style.data(), style.size());

		auto draw = [&](const char* a_call, float a_x, float a_y) {
			std::array<RE::GFxValue, 2> point{ RE::GFxValue{ static_cast<double>(a_x) }, RE::GFxValue{ static_cast<double>(a_y) } };
			(void)border.Invoke(a_call, nullptr, point.data(), point.size());
		};

		draw("moveTo", left, top);
		draw("lineTo", right, top);
		draw("lineTo", right, bottom);
		draw("lineTo", left, bottom);
		draw("lineTo", left, top);
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