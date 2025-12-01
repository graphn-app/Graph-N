
#ifdef __linux__
#include "platform/linux/main_linux.h"

namespace platform
{

bool MainRoutine_Linux::init()
{
#error Not implemented
}

int MainRoutine_Linux::run(flow_callback& /*callback*/, std::atomic_bool& /*request_focus*/)
{
#error Not implemented
}

void MainRoutine_Linux::free()
{
#error Not implemented
}

void MainRoutine_Linux::wnd_restore()
{
#error Not implemented
}

void MainRoutine_Linux::wnd_minimize()
{
#error Not implemented
}

void MainRoutine_Linux::wnd_maximize()
{
#error Not implemented
}

void MainRoutine_Linux::wnd_show()
{
#error Not implemented
}

void MainRoutine_Linux::wnd_hide()
{
#error Not implemented
}

} // namespace platform

#endif // #ifdef __linux__
