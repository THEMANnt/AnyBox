#include "Tests.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <algorithm>
#include <cstring>
#include <memory>

namespace {
struct Release {
    void operator()(void* value) const noexcept { if (!VirtualFree(value, 0, MEM_RELEASE)) std::terminate(); }
};
}

void AnyBox::Relinker::Tests::CheckExecution() {
    Decoder decoder;
    std::array<std::uint8_t, 9> source{0x49, 0x89, 0xC9, 0x66, 0x41, 0x0F, 0xAE, 0x31, 0xC3};
    auto code = std::as_bytes(std::span(source)).subspan(3, 5);
    auto replacement = decoder.LowerWriteback(decoder.Decode(code), code);
    Require(replacement.has_value(), "Native cache operation was not lowered");
    std::memcpy(source.data() + 3, replacement->bytes.data(), replacement->length);
    std::unique_ptr<void, Release> memory(VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!memory) Windows::ThrowLastError("Allocate native writeback test code");
    std::memcpy(memory.get(), source.data(), source.size());
    DWORD old = 0;
    if (!VirtualProtect(memory.get(), 4096, PAGE_EXECUTE_READ, &old)) Windows::ThrowLastError("Protect native writeback test code");
    if (!FlushInstructionCache(GetCurrentProcess(), memory.get(), source.size())) Windows::ThrowLastError("Flush native writeback test code");
    using Function = void (*)(void*);
    Function function;
    auto* address = memory.get();
    static_assert(sizeof(function) == sizeof(address));
    std::memcpy(&function, &address, sizeof(function));
    alignas(64) std::array<std::byte, 64> data;
    data.fill(std::byte{0x73});
    function(data.data());
    Require(std::ranges::all_of(data, [](auto value) { return value == std::byte{0x73}; }), "Lowered native cache operation modified its data");
}
