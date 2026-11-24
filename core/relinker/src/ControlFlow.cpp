#include "ControlFlow.hpp"

bool AnyBox::Relinker::FollowTransfer(const Image& image, const DecodedInstruction& instruction, std::uint32_t address, const std::unordered_set<std::uint32_t>& imports, std::vector<std::uint32_t>& entries) {
    auto append = [&](std::uint64_t target) {
        if (target < image.GetBase() || target - image.GetBase() > UINT32_MAX || !image.IsExecutable(static_cast<std::uint32_t>(target - image.GetBase()))) throw std::runtime_error("PE control flow target is not executable");
        entries.push_back(static_cast<std::uint32_t>(target - image.GetBase()));
    };
    for (std::size_t index = 0; index < instruction.native.operand_count_visible; ++index) {
        const auto& operand = instruction.operands[index];
        ZyanU64 target = 0;
        if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operand.imm.is_relative) {
            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&instruction.native, &operand, image.GetBase() + address, &target))) throw std::runtime_error("Cannot resolve PE control flow target");
            append(target);
            return true;
        }
        if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY || operand.size != 64 || operand.mem.base != ZYDIS_REGISTER_RIP || operand.mem.index != ZYDIS_REGISTER_NONE || (operand.mem.segment != ZYDIS_REGISTER_DS && operand.mem.segment != ZYDIS_REGISTER_SS)) continue;
        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&instruction.native, &operand, image.GetBase() + address, &target))) throw std::runtime_error("Cannot resolve PE indirect pointer address");
        if (target < image.GetBase() || target - image.GetBase() > UINT32_MAX) return false;
        auto slot = static_cast<std::uint32_t>(target - image.GetBase());
        if (imports.contains(slot)) return true;
        if (!image.IsReadOnly(slot, 8)) return false;
        append(Read<std::uint64_t>(image.GetBytes(), image.FileOffset(slot, 8)));
        return true;
    }
    return false;
}
