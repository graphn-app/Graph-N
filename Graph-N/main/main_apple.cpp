
#ifdef __APPLE__
#include "main/main_apple.h"

namespace platform
{

bool MainRoutine_Apple::init()
{
#error Not implemented
}

int32_t MainRoutine_Apple::run(flow_callback& /*callback*/, std::atomic_bool& /*request_focus*/)
{
#error Not implemented
}

void MainRoutine_Apple::free()
{
#error Not implemented
}

void MainRoutine_Apple::wnd_restore()
{
#error Not implemented
}

void MainRoutine_Apple::wnd_minimize()
{
#error Not implemented
}

void MainRoutine_Apple::wnd_maximize()
{
#error Not implemented
}

void MainRoutine_Apple::wnd_show()
{
#error Not implemented
}

void MainRoutine_Apple::wnd_hide()
{
#error Not implemented
}

} // namespace platform

#endif // #ifdef __APPLE__
