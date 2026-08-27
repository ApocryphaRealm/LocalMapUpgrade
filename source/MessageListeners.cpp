#include "Settings.h"

#include "Diagnostics.h"

#include "ShaderManager.h"
#include "ExtraMarkersManager.h"
#include "PlayerSetMarkerManager.h"

#include "FullAPI.h"

#include "IUI/API.h"

#include "UI.h"

const SKSE::LoadInterface* skse;

namespace LMU
{
	bool isIconDisplayExtensionPatched = false;
}

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg);

void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	// If all plugins have been loaded
	if (a_msg->type == SKSE::MessagingInterface::kPostLoad)
	{
		// DevBenchAPI's own contract: the interface can only be requested once SKSE has sent
		// kPostLoad, since that's the earliest point every plugin (DevBench included) has had its
		// own SKSEPluginLoad run. Done before the Infinity UI check below so the hook-install
		// results recorded during SKSEPluginLoad are queryable even in the sessions where that
		// check is about to end the process.
		logger::debug("kPostLoad received; registering live diagnostics with DevBench if present");
		diagnostics::Init();

		if (SKSE::GetMessagingInterface()->RegisterListener("InfinityUI", InfinityUIMessageListener))
		{
			diagnostics::RecordInfinityUIListenerRegistered(true);

			logger::info("Successfully registered for Infinity UI messages!");
		}
		else
		{
			diagnostics::RecordInfinityUIListenerRegistered(false);

			SKSE::stl::report_and_fail
			(
				std::format
				(
					"\n\n"
					"\"Infinity UI\" installation not detected.\n\n"
					"Please, download it from:\n"
					"www.nexusmods.com/skyrimspecialedition/mods/74483"
				)
			);
		}
	}
	// If the data handler has loaded all its forms
	else if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) 
	{
		LMU::ShaderManager::InitSingleton();
		diagnostics::RecordShaderManagerInitialized();

		LMU::API::PixelShaderPropertiesHookMessage pixelShaderPropertiesHook;
		pixelShaderPropertiesHook.SetPixelShaderProperties = &LMU::ShaderManager::SetPixelShaderProperties;
		pixelShaderPropertiesHook.GetPixelShaderProperties = &LMU::ShaderManager::GetPixelShaderProperties;
		DispatchMessage(pixelShaderPropertiesHook);

		// Consumers (Dragon's Eye Minimap) only ever receive the two function pointers through
		// this one dispatch. If it never fires, the minimap behaves exactly as if this mod were
		// not installed - so record that it did, rather than inferring it from the minimap's
		// symptoms later.
		diagnostics::RecordPixelShaderPropertiesHookDispatched();

		LMU::ExtraMarkersManager::InitSingleton();
		diagnostics::RecordExtraMarkersManagerInitialized();

		// Last retry point - if DevBench still isn't found here, conclude it isn't installed and
		// say so, rather than staying silent about it forever.
		diagnostics::Init(/* a_lastAttempt = */ true);
	}
	// Once every SKSE plugin (including SKSE Menu Framework itself) has finished loading.
	else if (a_msg->type == SKSE::MessagingInterface::kPostPostLoad)
	{
		UI::Register();

		// Rule-17 retry: a real launch showed devbench's own server can still be finishing
		// startup a moment after kPostLoad fires, which is early enough to lose the race even
		// though kPostLoad is DevBenchAPI's own documented earliest-safe point. Cheap no-op if
		// the kPostLoad attempt already succeeded.
		diagnostics::Init();
	}
}

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	using namespace IUI;

	if (!a_msg || std::string_view(a_msg->sender) != "InfinityUI")
	{
		return;
	}

	if (auto message = API::TranslateAs<API::Message>(a_msg))
	{
		std::string_view movieUrl = message->movie->GetMovieDef()->GetFileURL();

		if (movieUrl.find("Map") == std::string::npos)
		{
			return;
		}

		switch (a_msg->type)
		{
		case API::Message::Type::kStartLoadInstances:

			logger::info("Started loading patches");
			break;

		case API::Message::Type::kPostPatchInstance:

			if (auto msg = API::TranslateAs<API::PostPatchInstanceMessage>(a_msg))
			{				
				RE::GFxValue iconDisplayExtension;
				msg->newInstance._objectInterface->_movieRoot->GetVariable(&iconDisplayExtension, LMU::ExtraMarkersManager::extensionPath.data());

				if (msg->newInstance._value.obj == iconDisplayExtension._value.obj)
				{
					LMU::isIconDisplayExtensionPatched = true;

					diagnostics::RecordIconDisplayExtensionPatched();
				}
			}
			break;

		case API::Message::Type::kFinishLoadInstances:

			logger::info("Finished loading HUD patches");

			if (!LMU::isIconDisplayExtensionPatched)
			{
				SKSE::stl::report_and_fail
				(
					std::format
					(
						"\n\n"
						"\"Data\\Interface\\InfinityUI\\Map\\WorldMap\\LocalMapMenu\\IconDisplayExtension.swf\" not found.\n"
						"Please, check your installation files."
					)
				);
			}
			break;

		case API::Message::Type::kPostInitExtensions:

			if (auto msg = API::TranslateAs<API::PostInitExtensionsMessage>(a_msg))
			{
				logger::debug("Extensions initialization finished");
			}
			break;

		default:
			break;
		}
	}
}