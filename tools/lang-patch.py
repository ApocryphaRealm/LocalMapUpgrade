# -*- coding: utf-8 -*-
"""lang-patch.py - one-shot, re-runnable language-support patch for Local Map Upgrade.

Applies the consumer-side mechanism from D:\\Claude output\\4. plans\\translation-rollout\\plan.md
sections 2 and 4.1: strings::TR() routing for every literal the settings page draws, the
"!ApocryphaMenuFramework" module-name lookup, strings::Configure() at kDataLoaded, and a
"strings" op on the "localmapupgrade.status" DevBench tool. Every edit below is a must-match
anchor replace: if an anchor is not found EXACTLY ONCE the script raises instead of silently
doing nothing, so a stale run against changed source fails loudly rather than leaving the code
half patched.

Run from anywhere: `python tools/lang-patch.py` (paths are relative to the repo root, taken as
this script's grandparent directory).
"""
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read(path):
    with open(path, "r", encoding="utf-8", newline=None) as f:
        return f.read()


def write(path, text, crlf=False):
    with open(path, "w", encoding="utf-8", newline="\r\n" if crlf else "\n") as f:
        f.write(text)


def apply_one(text, anchor, replacement, label, done_marker=None):
    if done_marker is not None and done_marker in text:
        return text
    n = text.count(anchor)
    if n != 1:
        raise RuntimeError("[{}] anchor found {} time(s), expected exactly 1:\n{!r}".format(label, n, anchor))
    return text.replace(anchor, replacement, 1)


# ------------------------------------------------------------------------------------------------
# 1) include/SKSEMenuFramework.h - "!ApocryphaMenuFramework" first, ahead of the direct name.
# ------------------------------------------------------------------------------------------------
def patch_skse_menu_framework_h():
    path = os.path.join(REPO, "include", "SKSEMenuFramework.h")
    text = read(path)
    if 'GetModuleHandleW(L"!ApocryphaMenuFramework")' in text:
        return  # already applied by a previous (partial) run
    anchor = (
        "inline HMODULE GetMenuFrameworkModule() {\n"
        "    static HMODULE menuFramework = nullptr;\n"
        "    if (!menuFramework) {\n"
        "        menuFramework = GetModuleHandleW(L\"ApocryphaMenuFramework\");\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"SKSEMenuFramework\");\n"
        "        }\n"
        "    }\n"
        "    return menuFramework;\n"
        "}"
    )
    replacement = (
        "inline HMODULE GetMenuFrameworkModule() {\n"
        "    static HMODULE menuFramework = nullptr;\n"
        "    if (!menuFramework) {\n"
        "        menuFramework = GetModuleHandleW(L\"!ApocryphaMenuFramework\");\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"ApocryphaMenuFramework\");\n"
        "        }\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"SKSEMenuFramework\");\n"
        "        }\n"
        "    }\n"
        "    return menuFramework;\n"
        "}"
    )
    text = apply_one(text, anchor, replacement, "SKSEMenuFramework.h:GetMenuFrameworkModule")
    write(path, text, crlf=True)


# ------------------------------------------------------------------------------------------------
# 2) source/MessageListeners.cpp - strings::Configure("LocalMapUpgrade") at kDataLoaded.
# ------------------------------------------------------------------------------------------------
def patch_message_listeners_cpp():
    path = os.path.join(REPO, "source", "MessageListeners.cpp")
    text = read(path)
    if 'strings::Configure("LocalMapUpgrade")' in text:
        return  # already applied by a previous (partial) run

    text = apply_one(
        text,
        "#include \"UI.h\"\n\nconst SKSE::LoadInterface* skse;",
        "#include \"UI.h\"\n#include \"utils/Strings.h\"\n\nconst SKSE::LoadInterface* skse;",
        "MessageListeners.cpp:include",
    )

    anchor = (
        "\t// If the data handler has loaded all its forms\n"
        "\telse if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) \n"
        "\t{\n"
        "\t\tLMU::ShaderManager::InitSingleton();\n"
    )
    replacement = (
        "\t// If the data handler has loaded all its forms\n"
        "\telse if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) \n"
        "\t{\n"
        "\t\tstrings::Configure(\"LocalMapUpgrade\");\n"
        "\n"
        "\t\tLMU::ShaderManager::InitSingleton();\n"
    )
    text = apply_one(text, anchor, replacement, "MessageListeners.cpp:kDataLoaded")
    write(path, text, crlf=True)


# ------------------------------------------------------------------------------------------------
# 3) source/Diagnostics.cpp - a "strings" op on "localmapupgrade.status" returning
#    strings::StatusJson(); descriptor updated.
# ------------------------------------------------------------------------------------------------
def patch_diagnostics_cpp():
    path = os.path.join(REPO, "source", "Diagnostics.cpp")
    text = read(path)

    text = apply_one(
        text,
        "#include \"DevBench/DevBenchAPI.h\"\n#include \"Settings.h\"\n#include \"utils/Logger.h\"\n",
        "#include \"DevBench/DevBenchAPI.h\"\n#include \"Settings.h\"\n#include \"utils/Logger.h\"\n#include \"utils/Strings.h\"\n",
        "Diagnostics.cpp:include",
    )

    anchor = (
        "\t\t\t\ta_write(a_sink, reply.c_str());\n"
        "\t\t\t\treturn;\n"
        "\t\t\t}\n"
        "\n"
        "\t\t\tstd::string json;\n"
    )
    replacement = (
        "\t\t\t\ta_write(a_sink, reply.c_str());\n"
        "\t\t\t\treturn;\n"
        "\t\t\t}\n"
        "\t\t\tif (ArgString(args, \"op\") == \"strings\")\n"
        "\t\t\t{\n"
        "\t\t\t\tconst std::string stringsReply = std::format(R\"({{\"ok\":true,\"op\":\"strings\",\"strings\":{}}})\", strings::StatusJson());\n"
        "\t\t\t\ta_write(a_sink, stringsReply.c_str());\n"
        "\t\t\t\treturn;\n"
        "\t\t\t}\n"
        "\n"
        "\t\t\tstd::string json;\n"
    )
    text = apply_one(text, anchor, replacement, "Diagnostics.cpp:StatusTool ops")

    text = apply_one(
        text,
        "\"zoom) plus player-set-marker counters. op=borderstyle with value 0 (knotwork) or 1 \"\n"
        "\t\t\t\"(untarnished) switches the map border's style live.\\\",\"",
        "\"zoom) plus player-set-marker counters. op=borderstyle with value 0 (knotwork) or 1 \"\n"
        "\t\t\t\"(untarnished) switches the map border's style live; op=strings reports the active \"\n"
        "\t\t\t\"language, source and loaded translation count.\\\",\"",
        "Diagnostics.cpp:descriptor",
    )
    write(path, text, crlf=True)


# ------------------------------------------------------------------------------------------------
# 4) source/UI.cpp - route every drawn literal through strings::TR().
# ------------------------------------------------------------------------------------------------
def patch_ui_cpp():
    path = os.path.join(REPO, "source", "UI.cpp")
    text = read(path)

    text = apply_one(
        text,
        "#include \"utils/Logger.h\"\n#include \"utils/Toggle.h\"",
        "#include \"utils/Logger.h\"\n#include \"utils/Strings.h\"\n#include \"utils/Toggle.h\"",
        "UI.cpp:include",
    )

    text = apply_one(
        text,
        "#include <algorithm>\n",
        "#include <algorithm>\n#include <string>\n#include <vector>\n",
        "UI.cpp:vector include",
    )

    # --- kLogLevelNames block: add the parallel key array right after it --------------------------
    text = apply_one(
        text,
        "\t\tconstexpr const char* kLogLevelNames[] = { \"Trace\", \"Debug\", \"Info\", \"Warning\", \"Error\", \"Critical\", \"Off\" };\n"
        "\t\tconstexpr int kLogLevelCount = 7;",
        "\t\tconstexpr const char* kLogLevelNames[] = { \"Trace\", \"Debug\", \"Info\", \"Warning\", \"Error\", \"Critical\", \"Off\" };\n"
        "\t\tconstexpr const char* kLogLevelKeys[] = { \"LMU_LogLevel_Trace\", \"LMU_LogLevel_Debug\", \"LMU_LogLevel_Info\",\n"
        "\t\t\t\t\t\t\t\t\t\t\t\t\t\"LMU_LogLevel_Warning\", \"LMU_LogLevel_Error\", \"LMU_LogLevel_Critical\", \"LMU_LogLevel_Off\" };\n"
        "\t\tconstexpr int kLogLevelCount = 7;",
        "UI.cpp:kLogLevelKeys",
    )

    # --- HelpMarker: the "(?)" indicator (the tooltip text passed in is TR'd at each call site) ---
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"(?)\");\n"
        "\n"
        "\t\t\tif (ImGuiMCP::IsItemHovered())\n"
        "\t\t\t{\n"
        "\t\t\t\tImGuiMCP::SetTooltip(\"%s\", a_description);\n"
        "\t\t\t}",
        "\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"%s\", strings::TR(\"LMU_HelpMark\", \"(?)\"));\n"
        "\n"
        "\t\t\tif (ImGuiMCP::IsItemHovered())\n"
        "\t\t\t{\n"
        "\t\t\t\tImGuiMCP::SetTooltip(\"%s\", a_description);\n"
        "\t\t\t}",
        "UI.cpp:HelpMarker",
    )

    # --- NudgeableSlider: the "<-->" nudge indicator ------------------------------------------------
    text = apply_one(
        text,
        "\t\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\t\tImGuiMCP::TextDisabled(\"<-->\");",
        "\t\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\t\tImGuiMCP::TextDisabled(\"%s\", strings::TR(\"LMU_SliderNudge\", \"<-->\"));",
        "UI.cpp:NudgeableSlider",
    )

    # --- RenderMapMenuSection ------------------------------------------------------------------------
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SeparatorText(\"Local map\");\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(\"Color\", &mapmenu::localMapColor))\n",
        "\t\t\tImGuiMCP::SeparatorText(strings::TR(\"LMU_Title\", \"Local map\"));\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(strings::TR(\"LMU_Color\", \"Color\"), &mapmenu::localMapColor))\n",
        "UI.cpp:MapMenu title+Color",
    )

    text = apply_one(
        text,
        "\t\t\tHelpMarker(\"Renders the local (dungeon/interior) map in color instead of the vanilla black-and-white.\");\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(\"Fog of war\", &mapmenu::localMapFogOfWar))\n",
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpColor\", \"Renders the local (dungeon/interior) map in color instead of the vanilla black-and-white.\"));\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(strings::TR(\"LMU_FogOfWar\", \"Fog of war\"), &mapmenu::localMapFogOfWar))\n",
        "UI.cpp:FogOfWar",
    )

    text = apply_one(
        text,
        "\t\t\tHelpMarker(\"Whether unexplored parts of the local map stay hidden. Disabling reveals the whole map.\");\n"
        "\n"
        "\t\t\tNudgeableSlider(\"Keyboard pan speed\", &mapmenu::localMapKeyboardPanSpeed, 5.0F, 300.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(\"How fast the local map pans when panning it with the keyboard.\");\n"
        "\n"
        "\t\t\tImGuiMCP::Spacing();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"Actor markers\");\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Show enemy actors\", &mapmenu::localMapShowEnemyActors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show hostile actors\", &mapmenu::localMapShowHostileActors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show guard actors\", &mapmenu::localMapShowGuardActors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show dead actors\", &mapmenu::localMapShowDeadActors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show teammate actors\", &mapmenu::localMapShowTeammateActors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show neutral actors\", &mapmenu::localMapShowNeutralActors);\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(\"Immersive mode\", &mapmenu::localMapShowActorsOnlyWithDetectSpell))\n",
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpFogOfWar\", \"Whether unexplored parts of the local map stay hidden. Disabling reveals the whole map.\"));\n"
        "\n"
        "\t\t\tNudgeableSlider(strings::TR(\"LMU_KeyboardPanSpeed\", \"Keyboard pan speed\"), &mapmenu::localMapKeyboardPanSpeed, 5.0F, 300.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpKeyboardPanSpeed\", \"How fast the local map pans when panning it with the keyboard.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::Spacing();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"%s\", strings::TR(\"LMU_ActorMarkers\", \"Actor markers\"));\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowEnemyActors\", \"Show enemy actors\"), &mapmenu::localMapShowEnemyActors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowHostileActors\", \"Show hostile actors\"), &mapmenu::localMapShowHostileActors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowGuardActors\", \"Show guard actors\"), &mapmenu::localMapShowGuardActors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowDeadActors\", \"Show dead actors\"), &mapmenu::localMapShowDeadActors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowTeammateActors\", \"Show teammate actors\"), &mapmenu::localMapShowTeammateActors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_ShowNeutralActors\", \"Show neutral actors\"), &mapmenu::localMapShowNeutralActors);\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(strings::TR(\"LMU_ImmersiveMode\", \"Immersive mode\"), &mapmenu::localMapShowActorsOnlyWithDetectSpell))\n",
        "UI.cpp:ActorMarkers block",
    )

    text = apply_one(
        text,
        "\t\t\tHelpMarker(\"Only shows actor markers on the local map while a detect life/dead effect is active, instead of always. Ships off by default - turn it on if you want markers gated behind a detect effect.\");\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Map border\", &mapmenu::localMapBorder);\n"
        "\t\t\tHelpMarker(\"Draws a frame around the local map. OFF by default: the game draws its own frame, and most UI replacers draw one too, so this would otherwise stack a second frame on somebody else's. Turn it on if your replacer removed the vanilla frame, or if you prefer ours. Applies live - no need to reopen the map.\");\n"
        "\n"
        "\t\t\tif (mapmenu::localMapBorder)\n"
        "\t\t\t{\n"
        "\t\t\t\tstatic const char* const kBorderStyles[] = { \"Skyrim\", \"Untarnished\" };\n"
        "\t\t\t\tint style = static_cast<int>(mapmenu::localMapBorderStyle);\n"
        "\t\t\t\tif (style < 0 || style > 1) { style = 0; }\n"
        "\t\t\t\tif (ImGuiMCP::Combo(\"Border style\", &style, kBorderStyles, 2))\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\tmapmenu::localMapBorderStyle = static_cast<std::uint32_t>(style);\n"
        "\t\t\t\t}\n"
        "\t\t\t\tHelpMarker(\"Skyrim: the knotwork frame - the same Nordic art the menu framework's Skyrim theme uses, drawn round the map. The default. Untarnished: a plain single line in Untarnished UI's off-white.\");\n"
        "\t\t\t}",
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpImmersiveMode\", \"Only shows actor markers on the local map while a detect life/dead effect is active, instead of always. Ships off by default - turn it on if you want markers gated behind a detect effect.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"LMU_MapBorder\", \"Map border\"), &mapmenu::localMapBorder);\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpMapBorder\", \"Draws a frame around the local map. OFF by default: the game draws its own frame, and most UI replacers draw one too, so this would otherwise stack a second frame on somebody else's. Turn it on if your replacer removed the vanilla frame, or if you prefer ours. Applies live - no need to reopen the map.\"));\n"
        "\n"
        "\t\t\tif (mapmenu::localMapBorder)\n"
        "\t\t\t{\n"
        "\t\t\t\t// \"Skyrim\" and \"Untarnished\" are art names (and folder identities) - left untranslated,\n"
        "\t\t\t\t// same as the game's other UI-replacer theme option lists.\n"
        "\t\t\t\tstatic const char* const kBorderStyles[] = { \"Skyrim\", \"Untarnished\" };\n"
        "\t\t\t\tint style = static_cast<int>(mapmenu::localMapBorderStyle);\n"
        "\t\t\t\tif (style < 0 || style > 1) { style = 0; }\n"
        "\t\t\t\tif (ImGuiMCP::Combo(strings::TR(\"LMU_BorderStyle\", \"Border style\"), &style, kBorderStyles, 2))\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\tmapmenu::localMapBorderStyle = static_cast<std::uint32_t>(style);\n"
        "\t\t\t\t}\n"
        "\t\t\t\tHelpMarker(strings::TR(\"LMU_HelpBorderStyle\", \"Skyrim: the knotwork frame - the same Nordic art the menu framework's Skyrim theme uses, drawn round the map. The default. Untarnished: a plain single line in Untarnished UI's off-white.\"));\n"
        "\t\t\t}",
        "UI.cpp:MapBorder+BorderStyle",
    )

    # --- RenderDebugSection: SeparatorText + Combo label + option list + HelpMarker ---------------
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SeparatorText(\"Debug\");\n"
        "\n"
        "\t\t\tint level = static_cast<int>(debug::logLevel);\n"
        "\t\t\tif (ImGuiMCP::Combo(\"Log level\", &level, kLogLevelNames, kLogLevelCount))\n"
        "\t\t\t{\n"
        "\t\t\t\tdebug::logLevel = static_cast<logger::level>(level);\n"
        "\n"
        "\t\t\t\tOnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Applies to the log immediately.\");",
        "\t\t\tImGuiMCP::SeparatorText(strings::TR(\"LMU_Debug\", \"Debug\"));\n"
        "\n"
        "\t\t\tint level = static_cast<int>(debug::logLevel);\n"
        "\t\t\t// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated\n"
        "\t\t\t// bytes for this call so the const char* pointers handed to Combo stay valid.\n"
        "\t\t\tstd::vector<std::string> logLevelLabelStore;\n"
        "\t\t\tlogLevelLabelStore.reserve(kLogLevelCount);\n"
        "\t\t\tfor (int i = 0; i < kLogLevelCount; ++i)\n"
        "\t\t\t{\n"
        "\t\t\t\tlogLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));\n"
        "\t\t\t}\n"
        "\t\t\tstd::vector<const char*> logLevelLabels;\n"
        "\t\t\tlogLevelLabels.reserve(logLevelLabelStore.size());\n"
        "\t\t\tfor (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }\n"
        "\t\t\tif (ImGuiMCP::Combo(strings::TR(\"LMU_LogLevel\", \"Log level\"), &level, logLevelLabels.data(), kLogLevelCount))\n"
        "\t\t\t{\n"
        "\t\t\t\tdebug::logLevel = static_cast<logger::level>(level);\n"
        "\n"
        "\t\t\t\tOnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpLogLevel\", \"Applies to the log immediately.\"));",
        "UI.cpp:RenderDebugSection",
    )

    # --- RenderButtons: Save / Reload / Restore buttons, their HelpMarkers, status assignments ----
    text = apply_one(
        text,
        "\t\t\tif (ImGuiMCP::Button(\"Save\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tstatusMessage = settings::Save() ? \"Settings saved.\" : \"Could not save the INI. See the log for why.\";\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Writes every setting above back to the INI. Comments and unrelated keys are left alone.\");\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(\"Reload from INI\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tif (settings::Reload())\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tApplyLiveSettings();\n"
        "\n"
        "\t\t\t\t\t\tstatusMessage = \"Settings reloaded from the INI.\";\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t\telse\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tstatusMessage = \"Could not read the INI. See the log for why.\";\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.\");\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(\"Restore defaults\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tsettings::RestoreDefaults();\n"
        "\t\t\t\t\tApplyLiveSettings();\n"
        "\t\t\t\t});\n"
        "\n"
        "\t\t\t\tstatusMessage = \"Defaults restored. Press Save to keep them.\";\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.\");",
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"LMU_SaveBtn\", \"Save\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tstatusMessage = settings::Save() ? strings::TR(\"LMU_StatusSaved\", \"Settings saved.\")\n"
        "\t\t\t\t\t\t\t\t\t\t\t\t\t   : strings::TR(\"LMU_StatusSaveFail\", \"Could not save the INI. See the log for why.\");\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpSave\", \"Writes every setting above back to the INI. Comments and unrelated keys are left alone.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"LMU_ReloadBtn\", \"Reload from INI\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tif (settings::Reload())\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tApplyLiveSettings();\n"
        "\n"
        "\t\t\t\t\t\tstatusMessage = strings::TR(\"LMU_StatusReloaded\", \"Settings reloaded from the INI.\");\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t\telse\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tstatusMessage = strings::TR(\"LMU_StatusReloadFail\", \"Could not read the INI. See the log for why.\");\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpReload\", \"Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"LMU_RestoreBtn\", \"Restore defaults\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tsettings::RestoreDefaults();\n"
        "\t\t\t\t\tApplyLiveSettings();\n"
        "\t\t\t\t});\n"
        "\n"
        "\t\t\t\tstatusMessage = strings::TR(\"LMU_StatusRestored\", \"Defaults restored. Press Save to keep them.\");\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"LMU_HelpRestore\", \"Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.\"));",
        "UI.cpp:RenderButtons",
    )

    # --- SettingsPanel::Render: strings::Tick() first, then the intro text ------------------------
    text = apply_one(
        text,
        "\tvoid __stdcall SettingsPanel::Render()\n"
        "\t{\n"
        "\t\tImGuiMCP::TextWrapped(\"Most settings apply as soon as you make them. Press Save to keep them for the next time you play.\");",
        "\tvoid __stdcall SettingsPanel::Render()\n"
        "\t{\n"
        "\t\tstrings::Tick();\n"
        "\n"
        "\t\tImGuiMCP::TextWrapped(\"%s\", strings::TR(\"LMU_Intro\", \"Most settings apply as soon as you make them. Press Save to keep them for the next time you play.\"));",
        "UI.cpp:Render Tick+Intro",
    )

    write(path, text, crlf=True)


def main():
    patch_skse_menu_framework_h()
    patch_message_listeners_cpp()
    patch_diagnostics_cpp()
    patch_ui_cpp()
    print("lang-patch.py: all anchors matched and patched.")


if __name__ == "__main__":
    main()
