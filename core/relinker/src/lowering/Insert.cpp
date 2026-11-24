#include <anybox/relinker/Lowering.hpp>
#include <initializer_list>
#include <stdexcept>
#include "VectorBuilder.hpp"

namespace {
unsigned Register(const ZydisDecodedOperand& operand) {
    if (operand.type != ZYDIS_OPERAND_TYPE_REGISTER || operand.reg.value < ZYDIS_REGISTER_XMM0 || operand.reg.value > ZYDIS_REGISTER_XMM15) throw std::invalid_argument("INSERTQ operand is not an AMD64 XMM register");
    return static_cast<unsigned>(operand.reg.value - ZYDIS_REGISTER_XMM0);
}

}

AnyBox::Relinker::LoweredFunction AnyBox::Relinker::LowerImmediateInsert(const DecodedInstruction& instruction, std::uint8_t stackSize) {
    if (stackSize != 32 && stackSize != 40) throw std::invalid_argument("INSERTQ helper requires a 32-byte or 40-byte stack frame");
    if (instruction.native.mnemonic != ZYDIS_MNEMONIC_INSERTQ || instruction.native.operand_count_visible != 4 || instruction.operands[2].type != ZYDIS_OPERAND_TYPE_IMMEDIATE || instruction.operands[3].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) throw std::invalid_argument("Lowering requires an immediate INSERTQ instruction");
    auto destination = Register(instruction.operands[0]);
    auto source = Register(instruction.operands[1]);
    auto length = static_cast<unsigned>(instruction.operands[2].imm.value.u & 63);
    auto index = static_cast<unsigned>(instruction.operands[3].imm.value.u & 63);
    if (length == 0) length = 64;
    if (length + index > 64) throw std::invalid_argument("INSERTQ field extends beyond its defined low quadword");

    std::array<unsigned, 2> scratch{};
    unsigned count = 0;
    for (unsigned reg = 0; reg < 6 && count < scratch.size(); ++reg) {
        if (reg != destination && reg != source) scratch[count++] = reg;
    }
    if (count != scratch.size()) throw std::logic_error("INSERTQ scratch register selection failed");
    auto value = scratch[0];
    auto mask = scratch[1];
    VectorBuilder builder;
    builder.Bytes({0x48, 0x8D, 0x64, 0x24, static_cast<std::uint8_t>(0x100 - stackSize)});
    builder.Spill(value, 0, false);
    builder.Spill(mask, 16, false);
    builder.Vector(0x66, 0x6F, value, source);
    builder.Vector(0x66, 0x76, mask, mask);
    builder.Shift(mask, 2, 64 - length);
    builder.Vector(0x66, 0xDB, value, mask);
    builder.Shift(value, 6, index);
    builder.Shift(mask, 6, index);
    builder.Vector(0x66, 0xDF, mask, destination);
    builder.Vector(0x66, 0xEB, value, mask);
    builder.Vector(0xF3, 0x7E, destination, value);
    builder.Spill(value, 0, true);
    builder.Spill(mask, 16, true);
    builder.Bytes({0x48, 0x8D, 0x64, 0x24, stackSize});
    auto epilog = static_cast<std::uint32_t>(builder.code.size());
    builder.Bytes({0xC3});
    return {std::move(builder.code), {{0, epilog, 5, stackSize}, {epilog, epilog + 1, 0, 0}}};
}
