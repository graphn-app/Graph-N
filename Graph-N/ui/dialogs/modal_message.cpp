/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/dialogs/modal_message.h"
#include "core/managers/settings_manager.h"
#include "main.h"

namespace ui
{

void ModalMessage::init(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def /*= ModalDlgCtrl::BtnOK*/)
{
    m_caption = std::move(caption) + DlgObj_SID[cast::to_szt(DlgObj::ModalMessage)];
    m_message = std::move(message);

    if (buttons & ModalDlgCtrl::IcoInfo)
    {
        m_color = g::pOptsManager->color_load(AppColor::MsgBoxInfo);
        m_icon = ICON_LC_INFO;
    }
    else if (buttons & ModalDlgCtrl::IcoQues)
    {
        m_color = g::pOptsManager->color_load(AppColor::MsgBoxQuestion);
        m_icon = ICON_LC_CIRCLE_QUESTION_MARK;
    }
    else if (buttons & ModalDlgCtrl::IcoWarn)
    {
        m_color = g::pOptsManager->color_load(AppColor::MsgBoxWarning);
        m_icon = ICON_LC_TRIANGLE_ALERT;
    }
    else
    {
        m_color = g::pOptsManager->color_load(AppColor::MsgBoxError);
        m_icon = ICON_LC_CIRCLE_X;
    }

    if (buttons & ModalDlgCtrl::BtnOK)
    {
        Button b;
        b.code = ModalDlgCtrl::BtnOK;
        b.text = "OK";
        m_buttons.push_back(std::move(b));
    }
    if (buttons & ModalDlgCtrl::BtnCancel)
    {
        Button b;
        b.code = ModalDlgCtrl::BtnCancel;
        b.text = "Cancel";
        m_buttons.push_back(std::move(b));
    }
    if (buttons & ModalDlgCtrl::BtnYes)
    {
        Button b;
        b.code = ModalDlgCtrl::BtnYes;
        b.text = "Yes";
        m_buttons.push_back(std::move(b));
    }
    if (buttons & ModalDlgCtrl::BtnNo)
    {
        Button b;
        b.code = ModalDlgCtrl::BtnNo;
        b.text = "No";
        m_buttons.push_back(std::move(b));
    }
    
    m_def = def;
    m_open_dlg = true;
}

ModalDlgCtrl ModalMessage::show()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g::pOptsManager->color_load(AppColor::MiscDlgBg));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(230, 230, 230, 255));
    ImGuiEx::PushDlgTitleStyles(m_color);

    if (m_open_dlg)
    {
        ImGui::OpenPopup(m_caption.c_str());
        m_open_dlg = false;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 200), ImVec2(1000, FLT_MAX));

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool show = true;
    if (!ImGui::BeginPopupModal(m_caption.c_str(), &show, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PopStyleColor(5);
        return m_return;
    }

    // icon
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, m_color);
    ImGui::PushFont(g::pMain->get_big_icons_font());

    //const float icon_width = ImGui::CalcTextSize(m_icon).x;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f); 
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50 / 2 );

    ImGui::TextUnformatted(m_icon);

    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

    // text
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextWrapped(m_message.c_str());
    ImGui::EndGroup();

    ImGuiEx::Spacing(10);

    // buttons
    constexpr float button_w = 100.0f;
    float total_buttons_width = m_buttons.size() * (button_w + ImGui::GetStyle().FramePadding.x * 2.0f);
    total_buttons_width += (m_buttons.size() - 1) * ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x - total_buttons_width) * 0.5f);

    ModalDlgCtrl ret = ModalDlgCtrl::Continue;
    for (int32_t i = 0; i < m_buttons.size(); i++)
    {
        if (i > 0)
            ImGui::SameLine();

        if (m_buttons[i].code == m_def && ImGui::IsWindowAppearing()) 
            ImGui::SetKeyboardFocusHere();         

        if (ImGui::Button(m_buttons[i].text, ImVec2(button_w, 0)))
        {
            ret = m_buttons[i].code;
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::Dummy(ImVec2(0, 20));

    if (ModalDlgCtrl::Continue == ret && (m_stop || ImGui::IsKeyPressed(ImGuiKey_Enter)))
    {
        ret = m_def;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    ImGui::PopStyleColor(5);
    return ret;
}

} // namespace ui
