#include "ExtraMarkersManager.h"

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

	void ExtraMarkersManager::AddExtraMarkers(RE::LocalMapMenu& a_localMapMenu)
	{
		logger::debug("AddExtraMarkers: enemy={} hostile={} guard={} dead={} teammate={} neutral={} immersive={} "
					 "(radii ft - alive={} undead={} dead={})",
			settings::mapmenu::localMapShowEnemyActors, settings::mapmenu::localMapShowHostileActors,
			settings::mapmenu::localMapShowGuardActors, settings::mapmenu::localMapShowDeadActors,
			settings::mapmenu::localMapShowTeammateActors, settings::mapmenu::localMapShowNeutralActors,
			settings::mapmenu::localMapShowActorsOnlyWithDetectSpell,
			GetAliveActorsDisplayRadius(), GetUndeadActorsDisplayRadius(), GetDeadActorsDisplayRadius());

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

		logger::debug("AddExtraMarkers: scanning {} nearby actor handle(s)", actorHandles.size());

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

		logger::debug("AddExtraMarkers: added {} marker(s), {} skipped by a visibility setting", addedCount, skippedBySettingCount);
	}

	void ExtraMarkersManager::PostCreateMarkers(RE::GFxValue& a_iconDisplay)
	{
		a_iconDisplay.Invoke("PostCreateMarkers");
	}
}