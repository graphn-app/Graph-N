
#pragma once
#include "core/managers/manager.h"

namespace ui
{
class PanelWorkflow;
}

namespace mngr
{

class HotkeysManager final : public Manager
{
public:
    HotkeysManager();
    ~HotkeysManager() override;

    bool init() override;
    void save(std::vector<ui::KeyBinding>&& hotkeys);    

    const std::vector<ui::KeyBinding>& get_key_bindings() const { return m_key_binding; }
    std::vector<ui::KeyBinding> get_default_key_bindings() const;

    std::vector<std::pair<ImGuiKey, const char*>> get_key_names() const;
    const char* get_key_binding_name(KeyTarget target, const char* append_to = nullptr) const;
    bool should_skip_target(KeyTarget target) const;

    bool process_hotkey() const;
    bool process_node_hotkey(ui::PanelWorkflow* wf_panel) const;

    bool test(const std::vector<ui::KeyBinding>& hotkeys, std::set<KeyTarget>& failed) const;

private:
    void fix_with_defaults();
    const ui::KeyBinding* get_active_key_binding() const;

private:
    std::vector<ui::KeyBinding> m_key_binding;
};

} // namespace mngr
