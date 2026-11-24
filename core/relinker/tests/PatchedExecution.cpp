#include "InsertFixture.hpp"
#include "Fixtures.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <memory>

namespace {
struct Release {
    void operator()(void* value) const noexcept { if (!VirtualFree(value, 0, MEM_RELEASE)) std::terminate(); }
};

struct Registration {
    PRUNTIME_FUNCTION functions;
    ~Registration() { if (!RtlDeleteFunctionTable(functions)) std::terminate(); }
};
}

void AnyBox::Relinker::Tests::CheckPatchedExecution(const Image& image, std::uint64_t expectedLow) {
    auto size = Read<std::uint32_t>(image.GetBytes(), 0xD0);
    std::unique_ptr<void, Release> memory(VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!memory) Windows::ThrowLastError("Allocate rewritten PE test image");
    auto* bytes = static_cast<std::byte*>(memory.get());
    std::memcpy(bytes, image.GetBytes().data(), Read<std::uint32_t>(image.GetBytes(), 0xD4));
    for (const auto& section : image.GetSections()) std::memcpy(bytes + section.address, image.GetBytes().data() + section.offset, section.size);
    DWORD old = 0;
    if (!VirtualProtect(memory.get(), size, PAGE_EXECUTE_READ, &old)) Windows::ThrowLastError("Protect rewritten PE test image");
    if (!FlushInstructionCache(GetCurrentProcess(), memory.get(), size)) Windows::ThrowLastError("Flush rewritten PE test image");
    auto directory = Read<std::uint32_t>(image.GetBytes(), 0x120);
    auto count = Read<std::uint32_t>(image.GetBytes(), 0x124) / 12;
    auto* functions = reinterpret_cast<PRUNTIME_FUNCTION>(bytes + directory);
    auto base = reinterpret_cast<DWORD64>(memory.get());
    if (!RtlAddFunctionTable(functions, count, base)) throw std::runtime_error("Register rewritten PE test unwind table");
    Registration registration{functions};
    InsertFixture fixture;
    InsertState state;
    for (unsigned reg = 0; reg < 16; ++reg) state.values[reg] = {0x123456789ABCDEF0ULL ^ reg, 0x15A735CC098FFE22ULL ^ reg};
    auto before = state.values;
    state.requested = 0xA93;
    fixture.RunAt(bytes + 0x1000, state);
    Require(state.values[0][0] == expectedLow, "Rewritten PE did not execute its SSE4a helper with its original operands");
    for (unsigned reg = 1; reg < 16; ++reg) Require(state.values[reg] == before[reg], "Rewritten PE INSERTQ helper modified an unrelated XMM register");

    alignas(16) std::array<std::uint64_t, 64> stack{};
    stack[33] = 0x12345678;
    auto site = Decoder().Decode(image.Code(0x1000)).native.mnemonic == ZYDIS_MNEMONIC_CALL ? 0x1000U : 0x1004U;
    auto callerStack = reinterpret_cast<DWORD64>(&stack[33]) - image.GetStackAllocation(site);
    *reinterpret_cast<DWORD64*>(callerStack - 8) = base + site + 5;
    CONTEXT context{};
    auto address = image.GetSections().back().address;
    auto first = Decoder().Decode(image.Code(address));
    if (first.native.mnemonic != ZYDIS_MNEMONIC_RET) address += first.native.length;
    context.Rip = base + address;
    context.Rsp = callerStack - 8 - image.GetStackAllocation(address);
    DWORD64 foundBase = 0;
    auto* helper = RtlLookupFunctionEntry(context.Rip, &foundBase, nullptr);
    Require(helper != nullptr && foundBase == base, "Windows did not find the appended helper unwind entry");
    PVOID handlerData = nullptr;
    DWORD64 frame = 0;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, context.Rip, helper, &context, &handlerData, &frame, nullptr);
    Require(context.Rip == base + site + 5 && context.Rsp == callerStack, "Appended PE unwind data did not recover its original call site");
    auto* original = RtlLookupFunctionEntry(context.Rip, &foundBase, nullptr);
    Require(original != nullptr && original->BeginAddress == 0x1000, "Original PE unwind entry was lost after adding the helper");
    RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, context.Rip, original, &context, &handlerData, &frame, nullptr);
    Require(context.Rip == stack[33] && context.Rsp == reinterpret_cast<DWORD64>(&stack[34]), "Unwinding from the appended helper through its original caller failed");
}
