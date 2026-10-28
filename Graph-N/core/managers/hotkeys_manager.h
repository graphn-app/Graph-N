/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/managers/manager.h"
#include "core/listeners.h"

namespace ui
{
class PanelWorkflow;
}

namespace data
{
class NodeInstance;
}

namespace mngr
{

class HotkeysManager final : public Manager
{
public:
    HotkeysManager() noexcept;
    ~HotkeysManager() noexcept override;

    bool init() override;
    void save(std::vector<ui::KeyBinding>&& hotkeys);

    void add_listener(listener::HotkeysChangedHandler* listener) { m_change_listeners.push_back(listener); }

    const std::vector<ui::KeyBinding>& get_key_bindings() const noexcept { return m_key_binding; }
    std::vector<ui::KeyBinding> get_default_key_bindings() const;

    const std::vector<std::pair<ImGuiKey, const char*>>& get_key_names() const;
    const char* get_key_binding_name(KeyTarget target, const char* append_to = nullptr) const;
    std::string get_key_binding_name_str(KeyTarget target) const;
    bool should_skip_target(KeyTarget target) const;

    ImGuiKey char_to_key(char key) const noexcept;
    char key_to_char(ImGuiKey key) const noexcept;

    bool process_hotkey() const;
    bool process_node_hotkey(const ui::PanelWorkflow* wf_panel) const;

    bool test(const std::vector<ui::KeyBinding>& hotkeys, std::set<KeyTarget>& failed) const;

private:
    void fix_with_defaults();
    const ui::KeyBinding* get_active_key_binding(const ui::PanelWorkflow* wf_panel) const;

private:
    std::vector<ui::KeyBinding> m_key_binding;
    std::vector<listener::HotkeysChangedHandler*> m_change_listeners;
};

} // namespace mngr
