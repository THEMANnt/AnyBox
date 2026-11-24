#include "VectorBuilder.hpp"
#include <stdexcept>

AnyBox::Relinker::LoweredFunction AnyBox::Relinker::LowerImmediateExtract(const DecodedInstruction& instruction) {
    if (instruction.native.mnemonic != ZYDIS_MNEMONIC_EXTRQ || instruction.native.operand_count_visible != 3 || instruction.operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE || instruction.operands[2].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) throw std::invalid_argument("Lowering requires an immediate EXTRQ instruction");
    const auto& destination = instruction.operands[0];
    if (destination.type != ZYDIS_OPERAND_TYPE_REGISTER || destination.reg.value < ZYDIS_REGISTER_XMM0 || destination.reg.value > ZYDIS_REGISTER_XMM15) throw std::invalid_argument("EXTRQ operand is not an AMD64 XMM register");
    auto reg = static_cast<unsigned>(destination.reg.value - ZYDIS_REGISTER_XMM0);
    auto length = static_cast<unsigned>(instruction.operands[1].imm.value.u & 63);
    auto index = static_cast<unsigned>(instruction.operands[2].imm.value.u & 63);
    if (length == 0) length = 64;
    if (length + index > 64) throw std::invalid_argument("EXTRQ field extends beyond its defined low quadword");
    VectorBuilder builder;
    if (index != 0) builder.Shift(reg, 2, index);
    if (length != 64) {
        builder.Shift(reg, 6, 64 - length);
        builder.Shift(reg, 2, 64 - length);
    }
    builder.Bytes({0xC3});
    auto size = static_cast<std::uint32_t>(builder.code.size());
    return {std::move(builder.code), {{0, size, 0, 0}}};
}
