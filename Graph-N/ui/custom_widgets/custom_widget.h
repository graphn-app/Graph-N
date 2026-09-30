#pragma once
#include "core/alive.h"

namespace data {
struct Pin;
class NodeInstance;
}

namespace ui
{

class CustomWidget final : public IsAlive
{
public:
    CustomWidget() noexcept;
    ~CustomWidget() noexcept;
    NO_COPY_MOVE(CustomWidget);

    void draw_custom_widget(data::NodeInstance* node, float pos, float width);
    void upload_edited_image(data::NodeInstance* node, std::filesystem::path&& image_path, bool image_is_mask);
    void reset_widgets();

    bool get_current_image_paths(data::NodeInstance* node, std::filesystem::path& path, std::filesystem::path& server_name) const;

    void add_output(wid wf_id, iid node_id, mngr::ExecTask::OutputType type, const std::string& data, int32_t preloaded_media_id);
    void cleanup_outputs(wid wf_id);
    void cleanup_outputs(wid wf_id, iid node_id);
    void copy_text_output(wid wf_id, iid node_id);
    void view_text_output(wid wf_id, iid node_id);

    bool check_open_def_app(data::NodeInstance* node, bool check_only);

private:
    struct OutputData;
    void draw_image_widget_from_output(OutputData& out);
    void draw_image_widget_from_source();
    void draw_video_widget_from_output(OutputData& out);
    void draw_video_widget_from_source();
    void draw_audio_widget_from_output(OutputData& out);
    void draw_audio_widget_from_source();
    void draw_text_widget_from_output(OutputData& out);

    struct Widget;
    void draw_widget_upload_button();
    void upload_media(Widget& widget, const std::filesystem::path& path, bool mask, bool overwrite);
    void check_media_list_version(Widget& widget);
    void draw_widget_message(const char* const lines[], size_t count);
    void draw_load_result_message(int32_t load_result);
    bool check_source_and_get_pin(Widget& widget, int64_t& index, data::Pin*& source_pin, bool skip_media_validation = false);
    void download_media(HttpRequestor requestor, Widget& widget, int64_t index);
    const char* format_timeline(float duration, float pos);

    struct WidgetItem;
    void draw_image(WidgetItem& widget);
    void draw_video(WidgetItem& widget);
    void draw_audio(WidgetItem& widget);

private:
    data::NodeInstance* m_node = nullptr;
    float m_start_pos = 0.0f;
    float m_widget_width = 0.0f;

    std::filesystem::path m_upload_file_path;

    struct WidgetItem final
    {
        std::filesystem::path temp_path;
        int32_t media_id = 0;
        bool requested = false;
        std::string or_size;
        float or_size_w = 0.0f;
        int32_t load_result = HTTP_RESP_OK;
        std::optional<bool> media_valid;
        std::optional<bool> media_can_play;
        mngr::VideoInfo video_info;
        mngr::AudioInfo audio_info;
        std::string text_info;
    };

    struct Widget final
    {
        NodeObjFlags media_type;

        iid upload_target = 0;
        std::string uploaded_media;

        std::vector<WidgetItem> items;
        std::vector<std::string> names;
        int32_t list_version = -1;
    };

    Widget m_images;
    Widget m_videos;
    Widget m_audios;

    struct OutputData
    {
        NodeObjFlags media_type;
        WidgetItem item;
    };
    struct PairHash
    {
        std::size_t operator()(const std::pair<wid, iid>& p) const
        {
            static_assert(std::is_same_v<wid, uint32_t>, "Update hash function");
            static_assert(std::is_same_v<iid, uint32_t>, "Update hash function");

            const auto h1 = std::hash<wid>{}(p.first);
            const auto h2 = std::hash<iid>{}(p.second);

            // boost simple hash combining technique
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
    std::unordered_map<std::pair<wid, iid>, OutputData, PairHash> m_outputs;
};

} // namespace ui
