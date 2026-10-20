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
    const std::vector<std::pair<ImGuiKey, const char*>>& get_key_names_for_bookmark() const;

    const char* get_key_binding_name(KeyTarget target, const char* append_to = nullptr) const;
    std::string get_key_binding_name_str(KeyTarget target) const;
    bool should_skip_target(KeyTarget target) const;

    bool process_hotkey() const;
    bool process_node_hotkey(const ui::PanelWorkflow* wf_panel) const;

    void register_bookmark(ImGuiKey key, data::NodeInstance* node) { unregister_bookmark(node); m_bookmarks[key] = node; }
    void unregister_bookmark(data::NodeInstance* node) { std::erase_if(m_bookmarks, [node](const std::pair<ImGuiKey, data::NodeInstance*>& pair) { return pair.second == node; }); }

    bool test(const std::vector<ui::KeyBinding>& hotkeys, std::set<KeyTarget>& failed) const;

private:
    void fix_with_defaults();
    const ui::KeyBinding* get_active_key_binding() const;

private:
    std::vector<ui::KeyBinding> m_key_binding;
    std::vector<listener::HotkeysChangedHandler*> m_change_listeners;
    std::unordered_map<ImGuiKey, data::NodeInstance*> m_bookmarks;
};

} // namespace mngr
