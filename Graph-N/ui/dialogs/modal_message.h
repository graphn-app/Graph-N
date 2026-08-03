#pragma once

namespace ui
{

class ModalMessage
{
public:
    NO_COPY_MOVE_CTORS(ModalMessage);

    void init(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def = ModalDlgCtrl::BtnOK);
    ModalDlgCtrl show();
    void update(std::string&& message) noexcept { m_message = std::move(message); }
    void stop() noexcept { m_stop = true; }

private:
    std::string m_caption; 
    std::string m_message;    
    const char* m_icon = nullptr;
    ImU32 m_color = 0;
    ModalDlgCtrl m_def = ModalDlgCtrl::BtnOK;
    bool m_show = false;
    bool m_stop = false;
    ModalDlgCtrl m_return = ModalDlgCtrl::BtnOK;
    
    struct Button final
    {
        ModalDlgCtrl code = _btn_none;
        const char* text = nullptr;
        float width = 100.0f;
    };
    std::vector<Button> m_buttons;
};

} // namespace ui
