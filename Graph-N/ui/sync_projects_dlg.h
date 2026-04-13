#pragma once
#include "ui/base_dlg.h"

namespace ui
{

class SyncProjectsDlg final : public BaseDlgWithData<DlgData>
{
public:
    SyncProjectsDlg();
    ~SyncProjectsDlg() override;

    void init() override;

private:
    DlgCode show(bool& focused) override;

private:

    enum class SyncAction 
    { 
        Skip = 0,
        UploadReplace,
        UploadCopy,
        DownloadReplace,
        DownloadCopy
    };

    enum class SyncStatus 
    {
        None,
        InProgress,
        Success,
        Error,
        Skipped
    };

    struct SyncRow 
    {
        std::string id;
        std::string name;

        bool exists_local = false;
        bool exists_server = false;

        time_t local_timestamp = 0;
        time_t server_timestamp = 0;

        std::string local_state_text;
        std::string server_state_text;

        SyncAction current_action = SyncAction::Skip;
        std::vector<SyncAction> available_actions;

        std::string new_name;
        SyncStatus status = SyncStatus::None;
    };

    const char* get_action_name(SyncAction action);
    void apply_recommended_actions();

private:
    std::vector<SyncRow> m_rows;
};


}
