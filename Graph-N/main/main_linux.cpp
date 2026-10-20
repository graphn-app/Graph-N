/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#include "main/main_linux.h"

namespace platform
{

bool MainRoutine_Linux::init()
{
#error Not implemented
}

int32_t MainRoutine_Linux::run(flow_callback& /*callback*/, std::atomic_bool& /*request_focus*/)
{
#error Not implemented
}

void MainRoutine_Linux::stop_and_cleanup()
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
