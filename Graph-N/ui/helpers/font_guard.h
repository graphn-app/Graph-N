/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace ui
{

struct FontGuard final
{
    bool pushed = false;
    ImFont* font = nullptr;

    FontGuard(ImFont* font_) noexcept
        : font(font_)
    {
        push();
    }
    ~FontGuard() noexcept
    {
        pop();
    }

    NO_COPY_MOVE(FontGuard);

    void push() noexcept
    {
        if (font != nullptr)
        {
            if (!pushed)
                ImGui::PushFont(font);
            pushed = true;
        }
    }
    void pop() noexcept
    {
        if (font != nullptr)
        {
            if (pushed)
                ImGui::PopFont();
            pushed = false;
        }
    }
};

} // namespace ui
