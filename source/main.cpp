#include "Hooks.h"
#include "Settings.h"

#include "utils/AddressLibraryGuard.h"
#include "utils/Logger.h"

extern const SKSE::LoadInterface* skse;
void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	// Workaround for static initialization order bug of CommonLibSSE-NG
	REL::Module::reset();
	
	skse = a_skse;

	const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton();

	if (!logger::init(plugin->GetName()))
	{
		return false;
	}

	logger::info("Loading {} {}...", plugin->GetName(), plugin->GetVersion());

	// Address Library pre-check (the guard every mod of ours carries), BEFORE SKSE::Init, which opens the
	// Address Library itself (logic library 6026): a missing file gets a message naming it and the plugin
	// loads inert instead of CommonLibSSE-NG's bare failure line.
	if (!AddressLibraryGuard::Guard("Local Map Upgrade"))
	{
		return true;
	}

	SKSE::Init(a_skse);

	settings::Init(std::string(plugin->GetName()) + ".ini");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);
	logger::describe_level(std::string(plugin->GetName()) + ".ini");

	if (!SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageListener))
	{
		return false;
	}

	hooks::Install();

	logger::set_level(logger::level::info, logger::level::info);
	logger::info("Succesfully loaded!");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

	return true;
}
