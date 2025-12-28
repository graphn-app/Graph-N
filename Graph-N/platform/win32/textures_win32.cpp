
#ifdef _WIN32
#include "platform/win32/textures_win32.h"
#include "platform/win32/main_win32.h"

namespace platform
{

ID3D11Device* Textures_Win32::get_device() const
{
    auto main = dynamic_cast<MainRoutine_Win32*>(m_main);
    return main->get_device();
}

} // namespace platform

#endif // #ifdef _WIN32
