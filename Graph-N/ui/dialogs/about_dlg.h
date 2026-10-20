/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/base_dlg.h"
#include "core/alive.h"

namespace ui
{

class AboutDlg final
    : public BaseDlgWithData<DlgData>
    , public IsAlive
{
public:
    AboutDlg(bool autocheck) noexcept;
    ~AboutDlg() noexcept override;

    void init() override;

private:
    DlgCode show(bool& focused) override;

private:
    std::filesystem::path m_ico_path;
    uint32_t m_ico_id = 0;
    std::string m_version;
    std::string m_release_date;

    std::string m_license;
    std::string m_third_party;
    std::string m_privacy;
    std::string m_readme;
    std::string* m_text_buffer = nullptr;

    bool m_autocheck = false;
    bool m_upd_checking = false;
    bool m_upd_checked = false;
    bool m_http_result = false;
    bool m_upd_found = false;
    std::string m_new_version;
};

} // namespace ui
