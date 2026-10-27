/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef _DEBUG
namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;
}

namespace ui
{

class DebugWfView final
{
public:
    NO_COPY_MOVE_CTORS(DebugWfView);

    void init(data::Workflow* wf);
    DlgCode show();

    const std::string& get_name() const noexcept { return m_name; }

private:
    void draw_wf_data();
    void draw_validation();
    void draw_nodes_list();
    void draw_links_list();

    void draw_pin_details(data::PinInstance* pin, bool is_input);
    std::string primitive_to_string(const Primitive& p) const;
    void draw_color_square(const char* label, ImU32 col) const;
    bool passes_filter(const std::string& text) const;

private:
    data::Workflow* m_wf = nullptr;
    static int32_t m_global_id;
    int32_t m_id = 0;
    std::string m_name;

    char m_filter[256] = {};
};

} // namespace ui
#endif
