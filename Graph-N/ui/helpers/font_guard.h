
#pragma once

namespace ui
{

struct FontGuard final
{
    bool pushed = false;
    ImFont* font = nullptr;

    FontGuard(ImFont* font_)
        : font(font_)
    {
        push();
    }
    ~FontGuard()
    {
        pop();
    }

    NO_COPY_MOVE(FontGuard);

    void push()
    {
        if (!pushed)
            ImGui::PushFont(font);
        pushed = true;
    }
    void pop()
    {
        if (pushed)
            ImGui::PopFont();
        pushed = false;
    }
};

} // namespace ui