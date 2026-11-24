#include "Tests.hpp"
#include "../../src/common/NativeApi.hpp"
#include "../../src/virtual/WindowsApi.hpp"

#include <array>
#include <exception>

namespace {

std::size_t MeasureCommittedStack() {
    using GetLimits = void (WINAPI*)(PULONG_PTR, PULONG_PTR);
    static const auto get = AnyBox::XMem::GetMemoryFunction<GetLimits>("GetCurrentThreadStackLimits");
    ULONG_PTR lower = 0;
    ULONG_PTR upper = 0;
    get(&lower, &upper);
    std::size_t result = 0;
    while (lower < upper) {
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(reinterpret_cast<const void*>(lower), &information, sizeof(information))) AnyBox::Windows::ThrowLastError("VirtualQuery stack test");
        if (information.State == MEM_COMMIT && (information.Protect & PAGE_GUARD) == 0) result += information.RegionSize;
        lower += information.RegionSize;
    }
    return result;
}

struct ThreadState {
    HANDLE ready;
    HANDLE finish;
    std::size_t stackSize = 0;
};

DWORD WINAPI RunThread(void* parameter) {
    auto& state = *static_cast<ThreadState*>(parameter);
    state.stackSize = MeasureCommittedStack();
    if (!SetEvent(state.ready)) std::terminate();
    if (WaitForSingleObject(state.finish, 5000) != WAIT_OBJECT_0) std::terminate();
    return 0;
}

}

void AnyBox::Tests::CheckStackStatistics() {
    XMemWorkingSetStatistics statistics{};
    XMemGetWorkingSetStatistics(0x20000004, &statistics);
    Require(statistics.stackUsed >= MeasureCommittedStack(), "XMem statistics omitted the current thread stack");
    Require(statistics.gameUsed == 0 && statistics.cpuPageTableSize == 0, "Stack query populated unrequested statistics");
    auto before = statistics.stackUsed;

    XMem::NativeHandle ready(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    XMem::NativeHandle finish(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!ready || !finish) Windows::ThrowLastError("CreateEventW stack test");
    ThreadState state{ready.get(), finish.get()};
    XMem::NativeHandle thread(CreateThread(nullptr, 0x40000, RunThread, &state, 0, nullptr));
    if (!thread) Windows::ThrowLastError("CreateThread stack test");
    auto readyResult = WaitForSingleObject(ready.get(), 5000);
    std::exception_ptr error;
    try {
        XMemGetWorkingSetStatistics(0x20000004, &statistics);
    } catch (...) {
        error = std::current_exception();
    }
    if (!SetEvent(finish.get())) Windows::ThrowLastError("SetEvent stack test");
    auto done = WaitForSingleObject(thread.get(), 5000);
    if (error) std::rethrow_exception(error);
    Require(readyResult == WAIT_OBJECT_0 && done == WAIT_OBJECT_0, "Stack test thread did not complete");
    Require(state.stackSize > 0 && statistics.stackUsed >= before + state.stackSize, "XMem statistics omitted another thread stack");
    auto during = statistics.stackUsed;
    XMemGetWorkingSetStatistics(0x20000004, &statistics);
    Require(statistics.stackUsed < during, "XMem statistics retained a terminated thread stack");
}
