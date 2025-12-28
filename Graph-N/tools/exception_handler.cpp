
#ifdef _WIN32

#include <eh.h>
#include <Shlwapi.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "tools/exception_handler.h"

namespace tools
{

class SehException final
{
private:
    unsigned int nSE = 0;

public:
    SehException() {}
    SehException(unsigned int n) :
        nSE(n)
    {}

    NO_COPY_MOVE(SehException);

    unsigned int getSeNumber() { return nSE; }
};

static void dump_mini_dump(HANDLE hdumpfile, PEXCEPTION_POINTERS excp_info)
{
    if (nullptr == excp_info)
    {
        // Generate exception to get proper context in dump
        __try
        {
            RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
        }
        __except (dump_mini_dump(hdumpfile, GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }
    else
    {
        MINIDUMP_EXCEPTION_INFORMATION eInfo;
        eInfo.ThreadId = GetCurrentThreadId();
        eInfo.ExceptionPointers = excp_info;
        eInfo.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hdumpfile,
            MINIDUMP_TYPE(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData),
            &eInfo, nullptr, nullptr);
    }
}

int __cdecl record_exception_info(PEXCEPTION_POINTERS pExceptPtrs, wchar_t* /*message*/)
{
    static std::atomic_flag bFirstTime = ATOMIC_FLAG_INIT;
    if (bFirstTime.test_and_set(std::memory_order_acquire))	// Going recursive! That must mean this routine crashed!
        return EXCEPTION_CONTINUE_SEARCH;

    PEXCEPTION_RECORD pRec = pExceptPtrs->ExceptionRecord;
    while (pRec)
    {
        if (pRec->ExceptionCode == STATUS_SINGLE_STEP)
            return EXCEPTION_CONTINUE_EXECUTION;
        pRec = pRec->ExceptionRecord;
    }

    auto crash_dump_path = fs::get_crash_dump_directory_path();
    tools::path::make_folder(crash_dump_path);
    crash_dump_path /= APP_CAPTION "_" + tt::dt::from_current_date().as_filename() + ".dmp";

    HANDLE hdump = CreateFileW(crash_dump_path.wstring().c_str(),
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hdump != INVALID_HANDLE_VALUE)
    {
        dump_mini_dump(hdump, pExceptPtrs);
        CloseHandle(hdump);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI SEHFilter(EXCEPTION_POINTERS* pExp)
{
    record_exception_info(pExp, L"SEHFilter");
    return EXCEPTION_EXECUTE_HANDLER;
}

void SEHTranslator(unsigned int, EXCEPTION_POINTERS * pExp)
{
    record_exception_info(pExp, L"SEHTranslator");
    throw SehException();
}

void set_exception_translator()
{
    _set_se_translator(SEHTranslator);
    SetUnhandledExceptionFilter(SEHFilter);
}

}	// namespace tools

#endif	// #ifdef _WIN32
