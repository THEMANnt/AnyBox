#include "Tests.hpp"
#include <vector>

void AnyBox::Relinker::Tests::CheckWriteback() {
    Decoder decoder;
    const std::vector<std::vector<std::uint8_t>> cases{
        {0x66, 0x41, 0x0F, 0xAE, 0x31},
        {0x66, 0x0F, 0xAE, 0x30},
        {0x64, 0x66, 0x0F, 0xAE, 0x30},
        {0x66, 0x65, 0x0F, 0xAE, 0x30},
        {0x48, 0x66, 0x0F, 0xAE, 0x30},
        {0x41, 0x66, 0x0F, 0xAE, 0x30},
        {0x66, 0x67, 0x41, 0x0F, 0xAE, 0x31},
        {0x66, 0x0F, 0xAE, 0x35, 0xFF, 0xFF, 0xFF, 0xFF},
        {0x64, 0x66, 0x41, 0x0F, 0xAE, 0x74, 0x24, 0x30},
        {0x66, 0x66, 0x41, 0x0F, 0xAE, 0x31}
    };
    for (const auto& bytes : cases) {
        auto code = std::as_bytes(std::span(bytes));
        auto original = decoder.Decode(code);
        Require(original.native.mnemonic == ZYDIS_MNEMONIC_CLWB, "Writeback fixture is not CLWB");
        auto replacement = decoder.LowerWriteback(original, code);
        Require(replacement && replacement->length == bytes.size(), "Writeback lowering changed the instruction length");
        auto decoded = decoder.Decode(std::span(replacement->bytes).first(replacement->length));
        Require(decoded.native.mnemonic == ZYDIS_MNEMONIC_CLFLUSH, "Writeback lowering did not produce CLFLUSH");
    }
    const std::array<std::uint8_t, 4> pause{0x66, 0x0F, 0xAE, 0xF1};
    auto code = std::as_bytes(std::span(pause));
    Require(!decoder.LowerWriteback(decoder.Decode(code), code), "TPAUSE register form was mistaken for CLWB");
}
