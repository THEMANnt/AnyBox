#include "InsertFixture.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <cstring>

namespace {
std::vector<std::byte> Wrapper(void* target) {
    std::vector<std::byte> code;
    auto bytes = [&](std::initializer_list<std::uint8_t> data) { for (auto value : data) code.push_back(static_cast<std::byte>(value)); };
    auto displacement = [&](std::uint32_t value) {
        auto data = std::as_bytes(std::span(&value, 1));
        code.insert(code.end(), data.begin(), data.end());
    };
    auto vector = [&](unsigned reg, unsigned offset, bool load) {
        bytes({0xF3});
        if (reg >= 8) bytes({0x44});
        bytes({0x0F, static_cast<std::uint8_t>(load ? 0x6F : 0x7F), static_cast<std::uint8_t>(0x81 | ((reg & 7) << 3))});
        displacement(offset);
    };
    bytes({0x48, 0x83, 0xEC, 0x28});
    for (unsigned reg = 6; reg < 16; ++reg) vector(reg, 256 + (reg - 6) * 16, false);
    bytes({0xFF, 0xB1});
    displacement(432);
    bytes({0x9D, 0x9C, 0x58, 0x48, 0x89, 0x81});
    displacement(416);
    for (unsigned reg = 0; reg < 16; ++reg) vector(reg, reg * 16, true);
    bytes({0x48, 0xB8});
    auto targetBytes = std::as_bytes(std::span(&target, 1));
    code.insert(code.end(), targetBytes.begin(), targetBytes.end());
    bytes({0xFF, 0xD0});
    bytes({0x9C, 0x58, 0x48, 0x89, 0x81});
    displacement(424);
    for (unsigned reg = 0; reg < 16; ++reg) vector(reg, reg * 16, false);
    for (unsigned reg = 6; reg < 16; ++reg) vector(reg, 256 + (reg - 6) * 16, true);
    bytes({0x48, 0x83, 0xC4, 0x28, 0xC3});
    AnyBox::Relinker::Tests::Require(code.size() < 1024, "INSERTQ test wrapper overlaps its generated helper");
    return code;
}
}

AnyBox::Relinker::Tests::InsertFixture::InsertFixture() {
    static_assert(offsetof(InsertState, before) == 416 && offsetof(InsertState, after) == 424 && offsetof(InsertState, requested) == 432);
    memory = VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory) Windows::ThrowLastError("Allocate INSERTQ lowering test code");
}

AnyBox::Relinker::Tests::InsertFixture::~InsertFixture() {
    if (!VirtualFree(memory, 0, MEM_RELEASE)) std::terminate();
}

void AnyBox::Relinker::Tests::InsertFixture::Run(const LoweredFunction& function, InsertState& state) {
    auto wrapper = Wrapper(static_cast<std::byte*>(memory) + 1024);
    Require(function.code.size() <= 4096 - 1024, "INSERTQ helper exceeds the executable test allocation");
    DWORD old = 0;
    if (!VirtualProtect(memory, 4096, PAGE_READWRITE, &old)) Windows::ThrowLastError("Write INSERTQ lowering test code");
    std::memcpy(memory, wrapper.data(), wrapper.size());
    std::memcpy(static_cast<std::byte*>(memory) + 1024, function.code.data(), function.code.size());
    invoke(state);
}

void AnyBox::Relinker::Tests::InsertFixture::RunAt(void* target, InsertState& state) {
    auto wrapper = Wrapper(target);
    DWORD old = 0;
    if (!VirtualProtect(memory, 4096, PAGE_READWRITE, &old)) Windows::ThrowLastError("Write INSERTQ lowering test wrapper");
    std::memcpy(memory, wrapper.data(), wrapper.size());
    invoke(state);
}

void AnyBox::Relinker::Tests::InsertFixture::invoke(InsertState& state) {
    DWORD old = 0;
    if (!VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &old)) Windows::ThrowLastError("Protect INSERTQ lowering test code");
    if (!FlushInstructionCache(GetCurrentProcess(), memory, 4096)) Windows::ThrowLastError("Flush INSERTQ lowering test code");
    using Function = void (*)(InsertState*);
    Function invoke;
    static_assert(sizeof(invoke) == sizeof(memory));
    std::memcpy(&invoke, &memory, sizeof(invoke));
    invoke(&state);
}

AnyBox::Relinker::DecodedInstruction AnyBox::Relinker::Tests::InsertInstruction(unsigned destination, unsigned source, unsigned length, unsigned index) {
    std::vector<std::uint8_t> bytes{0xF2};
    auto rex = 0x40 | (destination >= 8 ? 4 : 0) | (source >= 8 ? 1 : 0);
    if (rex != 0x40) bytes.push_back(static_cast<std::uint8_t>(rex));
    bytes.insert(bytes.end(), {0x0F, 0x78, static_cast<std::uint8_t>(0xC0 | ((destination & 7) << 3) | (source & 7)), static_cast<std::uint8_t>(length), static_cast<std::uint8_t>(index)});
    return Decoder().Decode(std::as_bytes(std::span(bytes)));
}
