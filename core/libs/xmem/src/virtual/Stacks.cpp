#include "Statistics.hpp"
#include "../common/NativeApi.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace {

struct ThreadInformation {
    LONG exitStatus;
    void* teb;
    std::array<HANDLE, 2> clientId;
    ULONG_PTR affinity;
    LONG priority;
    LONG basePriority;
};

static_assert(sizeof(ThreadInformation) == 48);
static_assert(offsetof(ThreadInformation, teb) == 8);

std::size_t ReadStackSize(HANDLE thread) {
    using QueryThread = LONG (WINAPI*)(HANDLE, ULONG, void*, ULONG, ULONG*);
    static const auto query = AnyBox::XMem::GetNativeFunction<QueryThread>("NtQueryInformationThread");
    ThreadInformation information{};
    ULONG length = 0;
    auto status = query(thread, 0, &information, sizeof(information), &length);
    if (status < 0) AnyBox::XMem::ThrowNativeStatus(status, "NtQueryInformationThread XMem stacks");
    if (length != sizeof(information)) throw std::runtime_error("Unexpected XMem thread information size");
    if (!information.teb) return 0;

    std::array<std::uintptr_t, 2> stack{};
    SIZE_T read = 0;
    auto* address = static_cast<const std::byte*>(information.teb) + sizeof(void*);
    if (!ReadProcessMemory(GetCurrentProcess(), address, stack.data(), sizeof(stack), &read)) {
        auto error = GetLastError();
        DWORD exitCode = 0;
        if (!GetExitCodeThread(thread, &exitCode)) AnyBox::Windows::ThrowLastError("GetExitCodeThread XMem stacks");
        if (exitCode != STILL_ACTIVE) return 0;
        throw std::system_error(static_cast<int>(error), std::system_category(), "ReadProcessMemory XMem stacks");
    }
    if (read != sizeof(stack) || stack[0] < stack[1]) throw std::runtime_error("Invalid XMem thread stack bounds");
    return stack[0] - stack[1];
}

}

std::uint32_t AnyBox::XMem::GetStackSize() {
    using GetNextThread = LONG (WINAPI*)(HANDLE, HANDLE, ACCESS_MASK, ULONG, ULONG, HANDLE*);
    static const auto nextThread = GetNativeFunction<GetNextThread>("NtGetNextThread");
    constexpr auto noMoreEntries = std::bit_cast<LONG>(0x8000001AU);
    NativeHandle thread;
    std::uint64_t total = 0;
    for (;;) {
        HANDLE next = nullptr;
        auto status = nextThread(GetCurrentProcess(), thread.get(), THREAD_QUERY_INFORMATION, 0, 0, &next);
        if (status == noMoreEntries) break;
        if (status < 0) ThrowNativeStatus(status, "NtGetNextThread XMem stacks");
        if (!next) throw std::runtime_error("XMem thread enumeration returned a null handle");
        thread.reset(next);
        total += ReadStackSize(thread.get());
    }
    if (total > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("XMem stack usage exceeds the ABI range");
    return static_cast<std::uint32_t>(total);
}
