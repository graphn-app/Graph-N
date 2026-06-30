
#include "core/managers/hotkeys_manager.h"
#include "core/managers/settings_manager.h"
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

HotkeysManager::HotkeysManager() = default;
HotkeysManager::~HotkeysManager() = default;

bool HotkeysManager::init()
{
    m_key_binding = g::pOptsManager->ui_hotkeys_load();
    fix_with_defaults();
    return true;
}

void HotkeysManager::save(std::vector<ui::KeyBinding>&& hotkeys)
{
    m_key_binding = std::move(hotkeys);
    fix_with_defaults();
    g::pOptsManager->ui_hotkeys_save(m_key_binding);
}

std::vector<ui::KeyBinding> HotkeysManager::get_default_key_bindings() const
{
    std::vector<ui::KeyBinding> hks;
    { ui::KeyBinding kb; kb.target = KeyTarget::App_Import;          kb.key = ImGuiKey::ImGuiKey_I;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::App_Options;         kb.key = ImGuiKey::ImGuiKey_O;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::App_Quit;            kb.key = ImGuiKey::ImGuiKey_Q;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::App_TogglePanels;    kb.key = ImGuiKey::ImGuiKey_GraveAccent;                                    hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::App_FullScreen;      kb.key = ImGuiKey::ImGuiKey_F11;                                            hks.push_back(kb); }
  //{ ui::KeyBinding kb; kb.target = KeyTarget::App_Command;                                                                                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::App_Restart;         kb.key = ImGuiKey::ImGuiKey_R;          kb.ctrl = true; kb.shift = true;    hks.push_back(kb); }

    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_1;                kb.key = ImGuiKey::ImGuiKey_1;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_2;                kb.key = ImGuiKey::ImGuiKey_2;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_3;                kb.key = ImGuiKey::ImGuiKey_3;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_4;                kb.key = ImGuiKey::ImGuiKey_4;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_5;                kb.key = ImGuiKey::ImGuiKey_5;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_6;                kb.key = ImGuiKey::ImGuiKey_6;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_7;                kb.key = ImGuiKey::ImGuiKey_7;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_8;                kb.key = ImGuiKey::ImGuiKey_8;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_9;                kb.key = ImGuiKey::ImGuiKey_9;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_Next;             kb.key = ImGuiKey::ImGuiKey_W;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Wf_Prev;             kb.key = ImGuiKey::ImGuiKey_W;          kb.ctrl = true;                     hks.push_back(kb); }

    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Nodes;     kb.key = ImGuiKey::ImGuiKey_N;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Models;    kb.key = ImGuiKey::ImGuiKey_M;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Projects;  kb.key = ImGuiKey::ImGuiKey_P;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Templates; kb.key = ImGuiKey::ImGuiKey_T;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Logs;      kb.key = ImGuiKey::ImGuiKey_L;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Stats;     kb.key = ImGuiKey::ImGuiKey_S;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Queue;     kb.key = ImGuiKey::ImGuiKey_Q;                                              hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Inspector; kb.key = ImGuiKey::ImGuiKey_I;                                              hks.push_back(kb); }
  //{ ui::KeyBinding kb; kb.target = KeyTarget::PanelOpen_Notes;                                                                                 hks.push_back(kb); }

    { ui::KeyBinding kb; kb.target = KeyTarget::Project_CreateNew;   kb.key = ImGuiKey::ImGuiKey_N;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Project_Rename;      kb.key = ImGuiKey::ImGuiKey_F2;                                             hks.push_back(kb); }

    { ui::KeyBinding kb; kb.target = KeyTarget::Queue_Pause;         kb.key = ImGuiKey::ImGuiKey_Pause;                                          hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Queue_Interrupt;     kb.key = ImGuiKey::ImGuiKey_I;          kb.ctrl = true; kb.shift = true;    hks.push_back(kb); }

    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Save;      kb.key = ImGuiKey::ImGuiKey_S;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Undo;      kb.key = ImGuiKey::ImGuiKey_Z;          kb.ctrl = true;                     kb.repeatable = true; hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Redo;      kb.key = ImGuiKey::ImGuiKey_Y;          kb.ctrl = true;                     kb.repeatable = true; hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_SelAll;    kb.key = ImGuiKey::ImGuiKey_A;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Focus;     kb.key = ImGuiKey::ImGuiKey_F;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_RunsInf;   kb.key = ImGuiKey::ImGuiKey_0;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs1;     kb.key = ImGuiKey::ImGuiKey_1;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs2;     kb.key = ImGuiKey::ImGuiKey_2;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs3;     kb.key = ImGuiKey::ImGuiKey_3;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs4;     kb.key = ImGuiKey::ImGuiKey_4;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs5;     kb.key = ImGuiKey::ImGuiKey_5;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs6;     kb.key = ImGuiKey::ImGuiKey_6;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs7;     kb.key = ImGuiKey::ImGuiKey_7;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs8;     kb.key = ImGuiKey::ImGuiKey_8;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Runs9;     kb.key = ImGuiKey::ImGuiKey_9;          kb.ctrl = true;                     hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_RunBack;   kb.key = ImGuiKey::ImGuiKey_Enter;                                          hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_RunFront;  kb.key = ImGuiKey::ImGuiKey_Enter;      kb.shift = true;                    hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_Close;     kb.key = ImGuiKey::ImGuiKey_Escape;     kb.shift = true;                    hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Workspace_NewNode;   kb.key = ImGuiKey::ImGuiKey_Space;                                          hks.push_back(kb); }    

    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Pin;            kb.key = ImGuiKey::ImGuiKey_P;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Collapse;       kb.key = ImGuiKey::ImGuiKey_E;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Group;          kb.key = ImGuiKey::ImGuiKey_G;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Always;         kb.key = ImGuiKey::ImGuiKey_None;      kb.enabled = false;                  kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Mute;           kb.key = ImGuiKey::ImGuiKey_M;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Bypass;         kb.key = ImGuiKey::ImGuiKey_B;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Remove;         kb.key = ImGuiKey::ImGuiKey_Delete;                                         kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Copy;           kb.key = ImGuiKey::ImGuiKey_C;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Paste;          kb.key = ImGuiKey::ImGuiKey_V;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Dup;            kb.key = ImGuiKey::ImGuiKey_D;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Unlink;         kb.key = ImGuiKey::ImGuiKey_U;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_Reroute;        kb.key = ImGuiKey::ImGuiKey_R;          kb.ctrl = true;                     kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_AlignLeft;      kb.key = ImGuiKey::ImGuiKey_LeftArrow;  kb.ctrl = true; kb.shift = true;    kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_AlignTop;       kb.key = ImGuiKey::ImGuiKey_UpArrow;    kb.ctrl = true; kb.shift = true;    kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_MoveLeft;       kb.key = ImGuiKey::ImGuiKey_LeftArrow;  kb.ctrl = true;                     kb.repeatable = true; kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_MoveRight;      kb.key = ImGuiKey::ImGuiKey_RightArrow; kb.ctrl = true;                     kb.repeatable = true; kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_MoveUp;         kb.key = ImGuiKey::ImGuiKey_UpArrow;    kb.ctrl = true;                     kb.repeatable = true; kb.node_related = true;  hks.push_back(kb); }
    { ui::KeyBinding kb; kb.target = KeyTarget::Node_MoveDown;       kb.key = ImGuiKey::ImGuiKey_DownArrow;  kb.ctrl = true;                     kb.repeatable = true; kb.node_related = true;  hks.push_back(kb); }

    std::set<KeyTarget> failed;
    if (!test(hks, failed))
    {
        LogInternalLogic("Invalid predefined hotkeys");
    }
    return hks;
}

void HotkeysManager::fix_with_defaults()
{
    const auto defs = get_default_key_bindings();

    for (const auto& k : m_key_binding)
    {
        const auto it = std::find_if(defs.begin(), defs.end(),
            [&k](const auto& d)
            {
                return d.target == k.target;
            }
        );

        if (it == defs.end())
        {
            Log("Delete existing key binding, target " << cast::to_i32(k.target));
        }
    }

    std::vector<ui::KeyBinding> new_bindings;
    new_bindings.reserve(defs.size()); 

    for (const auto& d : defs)
    {
        const auto it = std::find_if(m_key_binding.begin(), m_key_binding.end(),
            [&d](const auto& k)
            {
                return k.target == d.target;
            }
        );

        if (it != m_key_binding.end())
        {
            new_bindings.push_back(*it);
        }
        else
        {
            new_bindings.push_back(d);
            Log("Add new key binding, target " << cast::to_i32(d.target))
        }
    }

    m_key_binding = std::move(new_bindings);
}

bool HotkeysManager::test(const std::vector<ui::KeyBinding>& hotkeys, std::set<KeyTarget>& failed) const
{
    std::set<KeyTarget> dup1;
    std::map<std::tuple<bool, bool, bool, bool, ImGuiKey>, KeyTarget> dup2;

    for (auto& hk : hotkeys)
    {
        if (!dup1.insert(hk.target).second)
            failed.insert(hk.target);
        if (hk.enabled)
        {
            const auto key = std::make_tuple(hk.alt, hk.cmd, hk.ctrl, hk.shift, hk.key);

            if (ImGuiKey_Escape == hk.key && !hk.alt && !hk.cmd && !hk.ctrl && !hk.shift) // handled by ImGui
            {
                dup2[key] = hk.target;
                failed.insert(hk.target);
            }
            else
            {
                const auto it = dup2.find(key);
                if (it == dup2.end())
                    dup2[key] = hk.target;
                else
                {
                    failed.insert(it->second);
                    failed.insert(hk.target);

                    switch (hk.target)
                    {
                    case KeyTarget::Workspace_Runs1:
                    case KeyTarget::Workspace_Runs2:
                    case KeyTarget::Workspace_Runs3:
                    case KeyTarget::Workspace_Runs4:
                    case KeyTarget::Workspace_Runs5:
                    case KeyTarget::Workspace_Runs6:
                    case KeyTarget::Workspace_Runs7:
                    case KeyTarget::Workspace_Runs8:
                    case KeyTarget::Workspace_Runs9:
                        failed.insert(KeyTarget::Workspace_Runs1);
                        break;
                    }
                }
            }
        }
    }

    return failed.empty();
}

std::vector<std::pair<ImGuiKey, const char*>> HotkeysManager::get_key_names() const
{
    std::vector<std::pair<ImGuiKey, const char*>> key_names =
    {
        { ImGuiKey_None, "" },

        { ImGuiKey_A, "A" }, { ImGuiKey_B, "B" }, { ImGuiKey_C, "C" }, { ImGuiKey_D, "D" }, { ImGuiKey_E, "E" }, { ImGuiKey_F, "F" }, { ImGuiKey_G, "G" },
        { ImGuiKey_H, "H" }, { ImGuiKey_I, "I" }, { ImGuiKey_J, "J" }, { ImGuiKey_K, "K" }, { ImGuiKey_L, "L" }, { ImGuiKey_M, "M" }, { ImGuiKey_N, "N" },
        { ImGuiKey_O, "O" }, { ImGuiKey_P, "P" }, { ImGuiKey_Q, "Q" }, { ImGuiKey_R, "R" }, { ImGuiKey_S, "S" }, { ImGuiKey_T, "T" }, { ImGuiKey_U, "U" },
        { ImGuiKey_V, "V" }, { ImGuiKey_W, "W" }, { ImGuiKey_X, "X" }, { ImGuiKey_Y, "Y" }, { ImGuiKey_Z, "Z" },

        { ImGuiKey_0, "0" }, { ImGuiKey_1, "1" }, { ImGuiKey_2, "2" }, { ImGuiKey_3, "3" }, { ImGuiKey_4, "4" }, { ImGuiKey_5, "5" }, { ImGuiKey_6, "6" },
        { ImGuiKey_7, "7" }, { ImGuiKey_8, "8" }, { ImGuiKey_9, "9" },

        { ImGuiKey_PageUp, "Page Up" }, { ImGuiKey_PageDown, "Page Down" }, { ImGuiKey_Home, "Home" }, { ImGuiKey_End, "End" },
        { ImGuiKey_Insert, "Insert" }, { ImGuiKey_Delete, "Delete" }, { ImGuiKey_Backspace, "Backspace" },
        { ImGuiKey_Space, "Space" }, { ImGuiKey_Enter, "Enter" }, { ImGuiKey_Escape, "Escape" },
        { ImGuiKey_Pause, "Pause" }, { ImGuiKey_PrintScreen, "Print Screen" }, { ImGuiKey_NumLock, "Num Lock" }, { ImGuiKey_ScrollLock, "Scroll Lock" },

        { ImGuiKey_F1, "F1" }, { ImGuiKey_F2, "F2" }, { ImGuiKey_F3, "F3" }, { ImGuiKey_F4, "F4" }, { ImGuiKey_F5, "F5" }, { ImGuiKey_F6, "F6" },
        { ImGuiKey_F7, "F7" }, { ImGuiKey_F8, "F8" }, { ImGuiKey_F9, "F9" }, { ImGuiKey_F10, "F10" }, { ImGuiKey_F11, "F11" }, { ImGuiKey_F12, "F12" },

        { ImGuiKey_LeftArrow, "Left Arrow" }, { ImGuiKey_RightArrow, "Right Arrow" }, { ImGuiKey_UpArrow, "Up Arrow" }, { ImGuiKey_DownArrow, "Down Arrow" },

        { ImGuiKey_Comma, "Comma (,)" }, { ImGuiKey_Period, "Period (.)" }, { ImGuiKey_Semicolon, "Semicolon (;)" }, 
        { ImGuiKey_Minus, "Minus (-)" }, { ImGuiKey_Equal, "Equal (=)" },
        { ImGuiKey_KeypadAdd, "Numpad Add (+)" }, { ImGuiKey_KeypadMultiply, "Numpad Multiply (*)" },
        { ImGuiKey_Slash, "Slash (/)" }, { ImGuiKey_Backslash, "Backslash (\\)" },         
        { ImGuiKey_Apostrophe, "Apostrophe (')" }, { ImGuiKey_GraveAccent, "Tilde (`)" },
        { ImGuiKey_LeftBracket, "Left Bracket ([)" }, { ImGuiKey_RightBracket, "Right Bracket (])" }
    };
    return key_names;
}

const char* HotkeysManager::get_key_binding_name(KeyTarget target, const char* append_to /*= nullptr*/) const
{
    static fmt::memory_buffer text;

    static auto key_names = get_key_names();
    const ui::KeyBinding* bind = nullptr;
    for (auto& hk : m_key_binding)
    {
        if (hk.target == target)
        {
            if (!hk.enabled)
                return append_to;
            bind = &hk;
            break;
        }
    }

    if (nullptr == bind)
        return append_to;

    const char* keyname = nullptr;
    for (size_t i = 0; i < key_names.size(); i++)
    {
        if (key_names[i].first == bind->key)
        {
            keyname = key_names[i].second;
            break;
        }
    }

    if (nullptr == keyname)
    {
        LogInternalLogic("Key name not found for " << cast::to_i32(bind->key));
        return append_to;
    }

    text.clear();
    if (append_to != nullptr)
        fmt::format_to(std::back_inserter(text), "{}", append_to);
    if (bind->cmd)
        fmt::format_to(std::back_inserter(text), "Cmd+");
    if (bind->ctrl)
        fmt::format_to(std::back_inserter(text), "Ctrl+");
    if (bind->alt)
        fmt::format_to(std::back_inserter(text), "Alt+");
    if (bind->shift)
        fmt::format_to(std::back_inserter(text), "Shift+");
    fmt::format_to(std::back_inserter(text), "{}\0", keyname);
    text.push_back(0);

    return text.data();
}

bool HotkeysManager::should_skip_target(KeyTarget target) const
{
    static std::set<KeyTarget> skip;
    if (skip.empty())
    {
        skip.insert(KeyTarget::App_Command);
        skip.insert(KeyTarget::PanelOpen_Notes);

        skip.insert(KeyTarget::Workspace_Runs2);
        skip.insert(KeyTarget::Workspace_Runs3);
        skip.insert(KeyTarget::Workspace_Runs4);
        skip.insert(KeyTarget::Workspace_Runs5);
        skip.insert(KeyTarget::Workspace_Runs6);
        skip.insert(KeyTarget::Workspace_Runs7);
        skip.insert(KeyTarget::Workspace_Runs8);
        skip.insert(KeyTarget::Workspace_Runs9);

        const auto skip_range = [](KeyTarget from, KeyTarget to)
        {
            for (size_t t = cast::to_szt(from); t <= cast::to_szt(to); t++)
            {
                skip.insert(cast::to_enum<KeyTarget>(t));
            }
        };
        skip_range(KeyTarget::App_ReservedFirst, KeyTarget::App_ReservedLast);
        skip_range(KeyTarget::Wf_ReservedFirst, KeyTarget::Wf_ReservedLast);
        skip_range(KeyTarget::PanelOpen_ReservedFirst, KeyTarget::PanelOpen_ReservedLast);
        skip_range(KeyTarget::Project_ReservedFirst, KeyTarget::Project_ReservedLast);
        skip_range(KeyTarget::Queue_ReservedFirst, KeyTarget::Queue_ReservedLast);
        skip_range(KeyTarget::Workspace_ReservedFirst, KeyTarget::Workspace_ReservedLast);
        skip_range(KeyTarget::Node_ReservedFirst, KeyTarget::Node_ReservedLast);
    }
    return skip.count(target) > 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//////////     handling                                   /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

const ui::KeyBinding* HotkeysManager::get_active_key_binding() const
{
    const ImGuiIO& io = ImGui::GetIO();
    if (g::block_hotkeys_counter > 0 || io.WantTextInput)
        return nullptr;

    const auto check_key_pressed = [](const ui::KeyBinding& kb) -> bool
    {
        if (ImGui::IsKeyPressed(kb.key, kb.repeatable)) 
            return true;

        switch (kb.key)
        {
        case ImGuiKey_0:      case ImGuiKey_Keypad0:        return ImGui::IsKeyPressed(ImGuiKey_0, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad0, false);
        case ImGuiKey_1:      case ImGuiKey_Keypad1:        return ImGui::IsKeyPressed(ImGuiKey_1, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad1, false);
        case ImGuiKey_2:      case ImGuiKey_Keypad2:        return ImGui::IsKeyPressed(ImGuiKey_2, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad2, false);
        case ImGuiKey_3:      case ImGuiKey_Keypad3:        return ImGui::IsKeyPressed(ImGuiKey_3, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad3, false);
        case ImGuiKey_4:      case ImGuiKey_Keypad4:        return ImGui::IsKeyPressed(ImGuiKey_4, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad4, false);
        case ImGuiKey_5:      case ImGuiKey_Keypad5:        return ImGui::IsKeyPressed(ImGuiKey_5, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad5, false);
        case ImGuiKey_6:      case ImGuiKey_Keypad6:        return ImGui::IsKeyPressed(ImGuiKey_6, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad6, false);
        case ImGuiKey_7:      case ImGuiKey_Keypad7:        return ImGui::IsKeyPressed(ImGuiKey_7, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad7, false);
        case ImGuiKey_8:      case ImGuiKey_Keypad8:        return ImGui::IsKeyPressed(ImGuiKey_8, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad8, false);
        case ImGuiKey_9:      case ImGuiKey_Keypad9:        return ImGui::IsKeyPressed(ImGuiKey_9, false)      || ImGui::IsKeyPressed(ImGuiKey_Keypad9, false);
        case ImGuiKey_Period: case ImGuiKey_KeypadDecimal:  return ImGui::IsKeyPressed(ImGuiKey_Period, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadDecimal, false);
        case ImGuiKey_Slash:  case ImGuiKey_KeypadDivide:   return ImGui::IsKeyPressed(ImGuiKey_Slash, false)  || ImGui::IsKeyPressed(ImGuiKey_KeypadDivide, false);
        case ImGuiKey_Minus:  case ImGuiKey_KeypadSubtract: return ImGui::IsKeyPressed(ImGuiKey_Minus, false)  || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false);
        case ImGuiKey_Enter:  case ImGuiKey_KeypadEnter:    return ImGui::IsKeyPressed(ImGuiKey_Enter, false)  || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
        case ImGuiKey_Equal:  case ImGuiKey_KeypadEqual:    return ImGui::IsKeyPressed(ImGuiKey_Equal, false)  || ImGui::IsKeyPressed(ImGuiKey_KeypadEqual, false);        
        }
        return false;
    };

    const ui::KeyBinding* active_bind = nullptr;
    for (const auto& hk : m_key_binding)
    {
        if (!hk.enabled || hk.alt != io.KeyAlt || hk.cmd != io.KeySuper || hk.ctrl != io.KeyCtrl || hk.shift != io.KeyShift)
            continue;

        if (check_key_pressed(hk))
        {
            active_bind = &hk;
            break;
        }
    }

    return active_bind;
}

bool HotkeysManager::process_hotkey() const
{
    const ui::KeyBinding* active_bind = get_active_key_binding();
    if (nullptr == active_bind || active_bind->node_related)
        return false;

    switch (active_bind->target)
    {
    case KeyTarget::App_Import:
        if (g::pFlow->get_nodes_ready())
            g::pFlow->on_menu_import();
        return true;
    case KeyTarget::App_Quit:
        g::pFlow->on_menu_quit();
        return true;
    case KeyTarget::App_FullScreen:
        g::pFlow->on_menu_fullscreen();
        return true;
    //case KeyTarget::App_Command:
    //    g::pFlow->on_menu_command();
    //    return true;
    case KeyTarget::App_Restart:
        g::pFlow->on_menu_restart();
        return true;
    case KeyTarget::App_Options:
        g::pFlow->on_menu_options();
        return true;
    case KeyTarget::App_TogglePanels:
        g::pFlow->on_menu_panel_toggle_active();
        return true;
    case KeyTarget::Wf_1:
    case KeyTarget::Wf_2:
    case KeyTarget::Wf_3:
    case KeyTarget::Wf_4:
    case KeyTarget::Wf_5:
    case KeyTarget::Wf_6:
    case KeyTarget::Wf_7:
    case KeyTarget::Wf_8:
    case KeyTarget::Wf_9:
        {
            const size_t number = cast::to_szt(active_bind->target) - cast::to_szt(KeyTarget::Wf_1) + 1;
            g::pFlow->wf_panel_activate_sequential(number);
        }        
        return true;
    case KeyTarget::Wf_Next:
        g::pFlow->on_menu_next_project(true);
        return true;
    case KeyTarget::Wf_Prev:
        g::pFlow->on_menu_next_project(false);
        return true;
    case KeyTarget::PanelOpen_Nodes:
        g::pFlow->on_menu_panel_window(ui::ViewPanelNodes, true);
        return true;
    case KeyTarget::PanelOpen_Models:
        g::pFlow->on_menu_panel_window(ui::ViewPanelModels, true);
        return true;
    case KeyTarget::PanelOpen_Projects:
        g::pFlow->on_menu_panel_window(ui::ViewPanelWfList, true);
        return true;
    case KeyTarget::PanelOpen_Templates:
        g::pFlow->on_menu_panel_window(ui::ViewPanelTemplates, true);
        return true;
    case KeyTarget::PanelOpen_Logs:
        g::pFlow->on_menu_panel_window(ui::ViewPanelOutput, true);
        return true;
    case KeyTarget::PanelOpen_Stats:
        g::pFlow->on_menu_panel_window(ui::ViewPanelStats, true);
        return true;
    case KeyTarget::PanelOpen_Queue:
        g::pFlow->on_menu_panel_window(ui::ViewPanelQueue, true);
        return true;
    case KeyTarget::PanelOpen_Inspector:
        g::pFlow->on_menu_panel_window(ui::ViewPanelNodeInpector, true);
        return true;
    //case KeyTarget::PanelOpen_Notes:
    //    on_menu_panel_window(ui::ViewPanelNotes, true);
    //    return true;
    case KeyTarget::Project_CreateNew:
        g::pFlow->on_menu_activate_wf(nullptr);
        return true;
    case KeyTarget::Project_Rename:
        g::pFlow->on_menu_wf_rename();
        return true;
    case KeyTarget::Queue_Pause:
        g::pFlow->on_menu_queue_pause();
        return true;
    case KeyTarget::Queue_Interrupt:
        g::pFlow->on_menu_queue_interrupt();
        return true;
    case KeyTarget::Workspace_Save:
        g::pFlow->wf_save(false);
        return true;
    case KeyTarget::Workspace_Undo:
        g::pFlow->wf_undo(false);
        return true;
    case KeyTarget::Workspace_Redo:
        g::pFlow->wf_redo(false);
        return true;
    case KeyTarget::Workspace_SelAll:
        g::pFlow->wf_node_select_all();
        return true;
    case KeyTarget::Workspace_RunsInf:
        g::pFlow->wf_num_of_runs(0);
        return true;
    case KeyTarget::Workspace_Runs1:
    case KeyTarget::Workspace_Runs2:
    case KeyTarget::Workspace_Runs3:
    case KeyTarget::Workspace_Runs4:
    case KeyTarget::Workspace_Runs5:
    case KeyTarget::Workspace_Runs6:
    case KeyTarget::Workspace_Runs7:
    case KeyTarget::Workspace_Runs8:
    case KeyTarget::Workspace_Runs9:
        {
            const int32_t number = cast::to_i32(active_bind->target) - cast::to_i32(KeyTarget::Workspace_Runs1) + 1;
            g::pFlow->wf_num_of_runs(number);
        }
        return true;
    case KeyTarget::Workspace_RunBack:
        g::pFlow->wf_run(false);
        return true;
    case KeyTarget::Workspace_RunFront:
        g::pFlow->wf_run(true);
        return true;
    case KeyTarget::Workspace_Close:
        g::pFlow->wf_panel_close(nullptr);
        return true;
    case KeyTarget::Workspace_Focus:
        g::pFlow->wf_focus();
        return true;
    case KeyTarget::Workspace_NewNode:
        g::pFlow->wf_new_node();
        return true;
    }

    LogInternalLogic("Unhandled global hotkey target " << cast::to_i32(active_bind->target));
    return false;
}

bool HotkeysManager::process_node_hotkey(ui::PanelWorkflow* wf_panel) const
{
    if (g::pFlow->wf_get_active_panel() != wf_panel)
        return false;

    const ui::KeyBinding* active_bind = get_active_key_binding();
    if (nullptr == active_bind || !active_bind->node_related)
        return false;

    switch (active_bind->target)
    {
    case KeyTarget::Node_Pin:
        g::pFlow->wf_node_toggle_pin();
        return true;
    case KeyTarget::Node_Collapse:
        g::pFlow->wf_node_toggle_collapse();
        return true;
    case KeyTarget::Node_Group:
        g::pFlow->wf_node_group();
        return true;
    case KeyTarget::Node_Always:
        g::pFlow->wf_node_always();
        return true;
    case KeyTarget::Node_Mute:
        g::pFlow->wf_node_toggle_mute();
        return true;
    case KeyTarget::Node_Bypass:
        g::pFlow->wf_node_toggle_bypass();
        return true;
    case KeyTarget::Node_Remove:
        g::pFlow->wf_node_remove();
        return true;
    case KeyTarget::Node_Copy:
        g::pFlow->wf_node_copy();
        return true;
    case KeyTarget::Node_Paste:
        g::pFlow->wf_node_paste({});
        return true;
    case KeyTarget::Node_Dup:
        g::pFlow->wf_node_duplicate();
        return true;
    case KeyTarget::Node_Unlink:
        g::pFlow->wf_node_unlink();
        return true;
  //case KeyTarget::Node_Reroute:
  //    return true;
    case KeyTarget::Node_AlignLeft:
        g::pFlow->wf_node_align(true, false);
        return true;
    case KeyTarget::Node_AlignTop:
        g::pFlow->wf_node_align(false, true);
        return true;
    case KeyTarget::Node_MoveLeft:
        g::pFlow->wf_node_move(true, false, false, false);
        return true;
    case KeyTarget::Node_MoveRight:
        g::pFlow->wf_node_move(false, true, false, false);
        return true;
    case KeyTarget::Node_MoveUp:
        g::pFlow->wf_node_move(false, false, true, false);
        return true;
    case KeyTarget::Node_MoveDown:
        g::pFlow->wf_node_move(false, false, false, true);
        return true;
    }

    LogInternalLogic("Unhandled global hotkey target " << cast::to_i32(active_bind->target));
    return false;
}

} // namespace mngr
