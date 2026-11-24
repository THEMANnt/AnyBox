#include "Dead.hpp"
#include "../Environment.hpp"
#include <spirv/unified1/spirv.hpp>
#include <memory>
#include <stdexcept>

namespace {
spv_result_t Count(void* data, const spv_parsed_instruction_t* instruction) {
    auto& uses = *static_cast<std::vector<std::size_t>*>(data);
    for (unsigned index = 0; index < instruction->num_operands; ++index) {
        const auto& operand = instruction->operands[index];
        bool identifier = operand.type == SPV_OPERAND_TYPE_ID || operand.type == SPV_OPERAND_TYPE_TYPE_ID || operand.type == SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID || operand.type == SPV_OPERAND_TYPE_SCOPE_ID;
        if (!identifier) continue;
        if (operand.num_words != 1 || instruction->words[operand.offset] >= uses.size()) return SPV_ERROR_INVALID_ID;
        ++uses[instruction->words[operand.offset]];
    }
    return SPV_SUCCESS;
}
}

bool AnyBox::Shader::RemoveDeadSubgroupOperations(std::vector<std::uint32_t>& code, std::uint32_t vulkanVersion) {
    std::vector<std::size_t> uses(code.at(3));
    std::unique_ptr<spv_context_t, decltype(&spvContextDestroy)> context(spvContextCreate(GetSpirvEnvironment(vulkanVersion)), &spvContextDestroy);
    if (!context) throw std::bad_alloc();
    if (spvBinaryParse(context.get(), &uses, code.data(), code.size(), nullptr, Count, nullptr) != SPV_SUCCESS) throw std::runtime_error("Cannot analyze subgroup operation uses");
    std::vector<std::uint32_t> output(code.begin(), code.begin() + 5);
    bool removed = false;
    bool shuffleNeeded = false;
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        auto opcode = code[offset] & 65535;
        bool shuffle = opcode == spv::OpGroupNonUniformShuffle || opcode == spv::OpGroupNonUniformShuffleXor;
        bool pure = shuffle || opcode == spv::OpGroupNonUniformBallot;
        if (pure && uses.at(code[offset + 2]) == 0) removed = true;
        else {
            shuffleNeeded |= shuffle;
            output.insert(output.end(), code.begin() + offset, code.begin() + offset + count);
        }
        offset += count;
    }
    if (!shuffleNeeded) {
        for (std::size_t offset = 5; offset < output.size();) {
            auto count = output[offset] >> 16;
            if ((output[offset] & 65535) == spv::OpCapability && count == 2 && output[offset + 1] == spv::CapabilityGroupNonUniformShuffle) {
                output.erase(output.begin() + offset, output.begin() + offset + count);
                removed = true;
            } else offset += count;
        }
    }
    if (removed) code = std::move(output);
    return removed;
}
