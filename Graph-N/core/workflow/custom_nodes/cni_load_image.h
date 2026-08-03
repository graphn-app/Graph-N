#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"
#include "core/alive.h"

namespace data
{
class PinInstance;

struct CNI_LoadImage final
    : public CNI_BaseObj_T<CNI_LoadImage>
    , public IsAlive
{
public:
    using CNI_BaseObj_T<CNI_LoadImage>::CNI_BaseObj_T;
    ~CNI_LoadImage() noexcept = default;

    static std::unique_ptr<Node> cni_create_object(mngr::NodesManager* /*manager*/) noexcept { return nullptr; }
    static const char* cni_get_node_class() noexcept { return CNI_CLASS_LOAD_IMAGE; }

    bool cni_is_applicable() const noexcept override;
    void cni_draw_widget(PinInstance* pin, float pos, float width) override;

private:
    void draw_upload_button(float pos, float width);
    void refresh_inputs() noexcept;
    void check_image_list_version(PinInstance* pin);

private:
    struct CNI_Image final
    {
        std::string temp_path;
        int32_t image_id = 0;
        bool requested = false;
        std::string or_size;
        float or_size_w = 0.0f;
    };
    std::vector<CNI_Image> m_images;
    bool m_refresh_inputs = false;
    std::string m_select_image;
    int32_t m_image_list_version_local = 0;

protected:
    static std::vector<Primitive> g_image_list;
    static int32_t g_image_list_version;
};

} // namespace data
