#include <anybox/relinker/Decoder.hpp>
#include <algorithm>
#include <stdexcept>

namespace {
bool SameMemory(const ZydisDecodedOperand& first, const ZydisDecodedOperand& second) {
    if (first.type != ZYDIS_OPERAND_TYPE_MEMORY || second.type != ZYDIS_OPERAND_TYPE_MEMORY) return false;
    const auto& a = first.mem;
    const auto& b = second.mem;
    return first.size == second.size && a.type == b.type && a.segment == b.segment && a.base == b.base && a.index == b.index && a.scale == b.scale && a.disp.has_displacement == b.disp.has_displacement && a.disp.value == b.disp.value;
}
}

std::optional<AnyBox::Relinker::EncodedInstruction> AnyBox::Relinker::Decoder::LowerWriteback(const DecodedInstruction& instruction, std::span<const std::byte> code) const {
    const auto& original = instruction.native;
    if (original.mnemonic != ZYDIS_MNEMONIC_CLWB) return std::nullopt;
    if (original.length > code.size() || original.length > ZYDIS_MAX_INSTRUCTION_LENGTH || original.raw.modrm.offset >= original.length || original.raw.modrm.mod == 3 || original.raw.modrm.reg != 6 || original.operand_count_visible != 1) throw std::invalid_argument("CLWB instruction encoding is inconsistent");
    EncodedInstruction replacement;
    replacement.length = original.length;
    std::ranges::copy(code.first(original.length), replacement.bytes.begin());
    bool changed = false;
    for (std::size_t index = 0; index < original.raw.prefix_count; ++index) {
        if (original.raw.prefixes[index].value == 0x66) {
            replacement.bytes[index] = std::byte{0x40};
            changed = true;
        }
    }
    if (!changed) throw std::runtime_error("CLWB has no mandatory operand size prefix");
    replacement.bytes[original.raw.modrm.offset] |= std::byte{8};

    auto verified = Decode(std::span(replacement.bytes).first(replacement.length));
    if (verified.native.mnemonic != ZYDIS_MNEMONIC_CLFLUSH || verified.native.length != original.length || verified.native.operand_count_visible != 1 || !SameMemory(instruction.operands[0], verified.operands[0])) throw std::runtime_error("CLWB lowering changed its effective memory operand");
    return replacement;
}
