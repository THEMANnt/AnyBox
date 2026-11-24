#include "Tests.hpp"
#include "../file/Tests.hpp"
#include "../../src/common/Handle.hpp"

#include <array>
#include <string>
#include <string_view>

namespace {

struct DebugCounts {
    unsigned messages = 0;
    unsigned breaks = 0;
    bool started = false;
};

void ReadMessage(HANDLE process, const OUTPUT_DEBUG_STRING_INFO& information, DebugCounts& counts) {
    if (information.fUnicode || information.nDebugStringLength == 0) return;
    std::array<char, 512> message{};
    auto size = std::min<std::size_t>(message.size(), information.nDebugStringLength);
    SIZE_T read = 0;
    if (!ReadProcessMemory(process, information.lpDebugStringData, message.data(), size, &read)) AnyBox::Windows::ThrowLastError("ReadProcessMemory debug message");
    std::string_view text(message.data(), read);
    if (text.starts_with("AnyBox DirectStorage: validation failed in ") && text.find("Enqueue") != std::string_view::npos) ++counts.messages;
}

DWORD HandleEvent(HANDLE process, const DEBUG_EVENT& event, DebugCounts& counts) {
    if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        if (event.u.CreateProcessInfo.hProcess != process) CloseHandle(event.u.CreateProcessInfo.hProcess);
        CloseHandle(event.u.CreateProcessInfo.hThread);
    } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        CloseHandle(event.u.CreateThread.hThread);
    } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
        if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
    } else if (event.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
        ReadMessage(process, event.u.DebugString, counts);
    } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
        if (event.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT) return DBG_EXCEPTION_NOT_HANDLED;
        if (counts.started) ++counts.breaks;
        counts.started = true;
    }
    return DBG_CONTINUE;
}

}

void AnyBox::Tests::CheckDebugSession(unsigned flags) {
    std::array<wchar_t, 32768> path{};
    auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0) Windows::ThrowLastError("GetModuleFileNameW debug test");
    Require(length < path.size(), "Debug test executable path was truncated");
    auto command = L"\"" + std::wstring(path.data(), length) + L"\" --child " + std::to_wstring(flags);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION information{};
    if (!CreateProcessW(path.data(), command.data(), nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &information)) Windows::ThrowLastError("CreateProcessW debug test");
    Storage::Handle process(information.hProcess);
    Storage::Handle thread(information.hThread);
    DebugCounts counts;
    DWORD exitCode = STILL_ACTIVE;
    for (;;) {
        DEBUG_EVENT event{};
        if (!WaitForDebugEvent(&event, 5000)) {
            TerminateProcess(process.Get(), 1);
            Windows::ThrowLastError("WaitForDebugEvent storage test");
        }
        auto continuation = HandleEvent(process.Get(), event, counts);
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) exitCode = event.u.ExitProcess.dwExitCode;
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation)) Windows::ThrowLastError("ContinueDebugEvent storage test");
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;
    }
    Require(exitCode == 0, "DirectStorage debug test child failed");
    Require(counts.messages == ((flags & 1) ? 4U : 0U), "DirectStorage diagnostic flag did not control debugger output");
    Require(counts.breaks == ((flags & 2) ? 4U : 0U), "DirectStorage break flag did not control validation breakpoints");
}
