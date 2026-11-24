#include "InsertFixture.hpp"
#include "Fixtures.hpp"
#include <anybox/relinker/Cpu.hpp>

namespace {
AnyBox::Relinker::DecodedInstruction Instruction(unsigned reg, unsigned length, unsigned index) {
    std::vector<std::uint8_t> bytes{0x66};
    if (reg >= 8) bytes.push_back(0x41);
    bytes.insert(bytes.end(), {0x0F, 0x78, static_cast<std::uint8_t>(0xC0 | (reg & 7)), static_cast<std::uint8_t>(length), static_cast<std::uint8_t>(index)});
    return AnyBox::Relinker::Decoder().Decode(std::as_bytes(std::span(bytes)));
}
}

void AnyBox::Relinker::Tests::CheckExtract() {
    InsertFixture fixture;
    Cpu host(CpuState::Detect());
    for (unsigned reg = 0; reg < 16; ++reg) {
        for (unsigned length = 1; length <= 64; ++length) {
            for (unsigned index = 0; index + length <= 64; ++index) {
                auto function = LowerImmediateExtract(Instruction(reg, length, index));
                InsertState state;
                for (unsigned at = 0; at < 16; ++at) state.values[at] = {0xB86F3421997ACE05ULL ^ (0x813742B9F964ED17ULL * at), 0x192773FAAACCB643ULL ^ at};
                auto before = state.values;
                auto mask = length == 64 ? UINT64_MAX : (std::uint64_t{1} << length) - 1;
                auto expected = (before[reg][0] >> index) & mask;
                state.requested = 0x202 | ((length ^ index) & 0xD5) | ((reg & 1) << 11);
                fixture.Run(function, state);
                Require(state.values[reg][0] == expected, "SSE2 EXTRQ lowering changed its defined low quadword");
                Require((state.before & 0x8D5) == (state.after & 0x8D5), "SSE2 EXTRQ lowering modified arithmetic flags");
                for (unsigned at = 0; at < 16; ++at) {
                    if (at != reg) Require(state.values[at] == before[at], "SSE2 EXTRQ lowering modified another XMM register");
                }
                for (std::size_t at = 0; at < function.code.size();) {
                    auto emitted = Decoder().Decode(std::span(function.code).subspan(at));
                    Require(host.Supports(emitted), "SSE2 EXTRQ lowering introduced an unsupported host instruction");
                    at += emitted.native.length;
                }
            }
        }
    }
    Require(LowerImmediateExtract(Instruction(0, 0, 0)).code == std::vector<std::byte>{std::byte{0xC3}}, "Full-width EXTRQ lowering emitted unnecessary instructions");
    Require(LowerImmediateExtract(Instruction(15, 255, 193)).code == LowerImmediateExtract(Instruction(15, 63, 1)).code, "EXTRQ lowering did not ignore the reserved high immediate bits");
    try { LowerImmediateExtract(Instruction(0, 0, 1)); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Undefined overflowing EXTRQ field was accepted");
}
