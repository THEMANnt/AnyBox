#include "InsertFixture.hpp"
#include <anybox/relinker/Cpu.hpp>

namespace {
void Check(AnyBox::Relinker::Tests::InsertFixture& fixture, unsigned destination, unsigned source, unsigned length, unsigned index) {
    using namespace AnyBox::Relinker;
    using namespace AnyBox::Relinker::Tests;
    auto function = LowerImmediateInsert(InsertInstruction(destination, source, length, index));
    InsertState state;
    for (unsigned reg = 0; reg < 16; ++reg) state.values[reg] = {0xF734A018C39D562BULL ^ (0x879C42A793E11965ULL * reg), 0x492E18AA617BF035ULL ^ reg};
    auto original = state.values;
    auto bits = length & 63;
    if (bits == 0) bits = 64;
    auto offset = index & 63;
    auto mask = bits == 64 ? UINT64_MAX : (std::uint64_t{1} << bits) - 1;
    auto expected = (original[destination][0] & ~(mask << offset)) | ((original[source][0] & mask) << offset);
    state.requested = 0x202 | ((length ^ index) & 0xD5) | ((destination & 1) << 11);
    fixture.Run(function, state);
    Require(state.values[destination][0] == expected, "SSE2 INSERTQ lowering changed the defined low quadword");
    Require((state.before & 0x8D5) == (state.after & 0x8D5), "SSE2 INSERTQ lowering modified arithmetic flags");
    for (unsigned reg = 0; reg < 16; ++reg) {
        if (reg != destination) Require(state.values[reg] == original[reg], "SSE2 INSERTQ lowering modified another XMM register");
    }
}
}

void AnyBox::Relinker::Tests::CheckInsert() {
    InsertFixture fixture;
    for (unsigned length = 1; length <= 64; ++length) {
        for (unsigned index = 0; index + length <= 64; ++index) Check(fixture, 0, 0, length, index);
    }
    for (unsigned destination = 0; destination < 16; ++destination) {
        for (unsigned source = 0; source < 16; ++source) {
            Check(fixture, destination, source, 8, 24);
            Check(fixture, destination, source, 0, 0);
            Check(fixture, destination, source, 127, 65);
        }
    }
    auto function = LowerImmediateInsert(InsertInstruction(15, 8, 8, 24));
    Cpu host(CpuState::Detect());
    Decoder decoder;
    for (std::size_t position = 0; position < function.code.size();) {
        auto instruction = decoder.Decode(std::span(function.code).subspan(position));
        Require(host.Supports(instruction), "SSE2 INSERTQ lowering introduced an unsupported host instruction");
        position += instruction.native.length;
    }
    CheckInsertUnwind(function);
    CheckInsertUnwind(LowerImmediateInsert(InsertInstruction(0, 0, 8, 24), 32));
    try { LowerImmediateInsert(InsertInstruction(0, 1, 0, 1)); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Undefined overflowing INSERTQ field was accepted");
}
