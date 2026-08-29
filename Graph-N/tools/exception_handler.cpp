#pragma once
#include "tools/exception_handler.h" 

#ifdef _WIN32

#include <eh.h>
#include <Shlwapi.h>
#include <dbghelp.h>
#include <atomic>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace tools
{

// Inheriting from std::exception is standard practice, even for SEH wrappers
class SehException : public std::exception
{
private:
    uint32_t nSE = 0;

public:
    SehException() noexcept = default;
    SehException(uint32_t n) noexcept : nSE(n) {}
    ~SehException() noexcept = default;

    // No need for NO_COPY_MOVE usually, exception needs to be copyable mostly
    uint32_t getSeNumber() const noexcept { return nSE; }

    const char* what() const noexcept override { return "SEH Exception (Access Violation, etc.)"; }

    static wchar_t dump_path[MAX_PATH * 2 + 1];
};

wchar_t SehException::dump_path[MAX_PATH * 2 + 1];

namespace
{
void dump_mini_dump(HANDLE hdumpfile, PEXCEPTION_POINTERS excp_info) noexcept
{
    MINIDUMP_EXCEPTION_INFORMATION eInfo = {};
    eInfo.ThreadId = ::GetCurrentThreadId();
    eInfo.ExceptionPointers = excp_info;
    eInfo.ClientPointers = FALSE;

    // Use nullptr for eInfo if excp_info is null, don't raise new exception
    PMINIDUMP_EXCEPTION_INFORMATION peInfo = excp_info ? &eInfo : nullptr;

    ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hdumpfile,
        MINIDUMP_TYPE(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData),
        peInfo, nullptr, nullptr);
}

// Helper to construct path without heap allocation if possible (simplified here)
// In a real strict handler, use wchar_t buffer[MAX_PATH] and GetModuleFileName
int32_t __cdecl record_exception_info(PEXCEPTION_POINTERS pExceptPtrs) noexcept
{
    static std::atomic_flag bFirstTime = ATOMIC_FLAG_INIT;
    if (bFirstTime.test_and_set(std::memory_order_acquire))
        return EXCEPTION_CONTINUE_SEARCH; // Prevent recursive crashes

    if (pExceptPtrs && pExceptPtrs->ExceptionRecord)
    {
        if (pExceptPtrs->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
            return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Attempt to flush logs. Wrap in try-catch because logger might use heap.
    try {
        glog::Logger::instance().force_flush();
    }
    catch (...) {}

    // Prepare path. 
    // WARNING: If Heap is corrupted, fs:: calls might hang. 
    // Ideally, pre-allocate dump folder path at startup and store in a static buffer.

    // Fallback logic for demo purposes (assuming your fs logic works):
    try
    {
        HANDLE hdump = ::CreateFileW(SehException::dump_path,
            GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);

        if (hdump != INVALID_HANDLE_VALUE)
        {
            dump_mini_dump(hdump, pExceptPtrs);
            ::CloseHandle(hdump);
        }
    }
    catch (...)
    {
        // Failed to create dump file
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


// 1. Unhandled Exception Filter (Top-level catch)
LONG WINAPI SEHFilter(EXCEPTION_POINTERS* pExp) noexcept
{
    record_exception_info(pExp);

    // Terminate process explicitly after dumping
    return EXCEPTION_EXECUTE_HANDLER;
}

// 2. Translator for converting SEH to C++ Exceptions (requires /EHa)
void SEHTranslator(uint32_t u, EXCEPTION_POINTERS* pExp)
{
    // Optional: You can record dump here too, OR wait until it hits the catch(...) block
    // Recording here is safer because stack is closer to the error.
    record_exception_info(pExp);

    throw SehException(u);
}
} // namespace

void set_exception_translator()
{
    auto crash_dump_path = fs::get_crash_dump_directory_path();
    tools::path::make_folder(crash_dump_path);
    crash_dump_path /= APP_CAPTION "_" + tools::dt::get_app_start_time().as_filename() + ".dmp";
    wcscpy_s(SehException::dump_path, MAX_PATH * 2, crash_dump_path.wstring().c_str());

    // Handle C++ try/catch for SEH
    _set_se_translator(SEHTranslator);

    // Handle crashes outside any try/catch
    ::SetUnhandledExceptionFilter(SEHFilter);

    // Prevent CRT from showing "Debug Error" dialogs in release
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
}

} // namespace tools

#endif // _WIN32
