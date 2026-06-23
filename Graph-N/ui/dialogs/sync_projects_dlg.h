#pragma once
#include "ui/base_dlg.h"
#include "core/alive.h"

namespace ui
{

class SyncProjectsDlg final :
    public BaseDlgWithData<DlgData>,
    public IsAlive
{
public:
    SyncProjectsDlg();
    ~SyncProjectsDlg() override;

    void init() override;

private:
    DlgCode show(bool& focused) override;

private:

    enum class Step       { Disconnected, Loading, LoadFailed, Ready, Sync };
    enum class SyncStatus { None, InProgress, Success, Error, Skipped };
    enum class SyncAction { Skip, Upload, UploadReplace, UploadRename, Download, DownloadReplace, DownloadRename, DeleteLocal, Undelete };

    struct SyncRow 
    {
        wid local_id = 0;
        bool deleted = false;
        bool cleanup_wf = false;

        std::string name;

        bool exists_local = false;
        bool exists_server = false;

        time_t local_timestamp = 0;
        std::string local_timestamp_str;
        time_t server_timestamp = 0;
        std::string server_timestamp_str;

        SyncAction selected_action = SyncAction::Skip;
        std::vector<SyncAction> available_actions;

        std::string new_name;
        bool new_name_invalid = false;
        SyncStatus status = SyncStatus::None;
    };

    struct Project
    {
        bool local = true;
        std::string name;
        wid local_id = 0;
        tools::dt time;
    };

    const char* get_action_name(SyncAction action);
    void apply_recommended_actions();
    void reset();
    void load_projects();
    void show_status_page();
    void setup_actions();
    void sync();

private:    
    Step m_step = Step::Disconnected;
    std::vector<SyncRow> m_rows;
    size_t m_current_row = 0;
};


}
