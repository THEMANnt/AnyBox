#include "Features.hpp"
#include <stdexcept>
#include <string>

AnyBox::Relinker::Cpu::Cpu(const CpuState& state) : tsc((state.basic[3] & (1U << 4)) != 0) {
    for (unsigned index = 1; index <= ZYDIS_ISA_SET_MAX_VALUE; ++index) {
        auto isa = static_cast<ZydisISASet>(index);
        auto scalar = GetScalarSupport(state, isa);
        auto vector = GetVectorSupport(state, isa);
        supported[index] = scalar.value_or(vector.value_or(false));
    }
}

bool AnyBox::Relinker::Cpu::Supports(const DecodedInstruction& instruction) const {
    const auto& native = instruction.native;
    if ((native.attributes & ZYDIS_ATTRIB_IS_PRIVILEGED) != 0) return false;
    if (native.mnemonic == ZYDIS_MNEMONIC_ENDBR32 || native.mnemonic == ZYDIS_MNEMONIC_ENDBR64) return true;
    if (native.mnemonic == ZYDIS_MNEMONIC_RDTSC && !tsc) return false;
    auto index = static_cast<unsigned>(native.meta.isa_set);
    return index < supported.size() && supported[index];
}

void AnyBox::Relinker::Cpu::Require(const DecodedInstruction& instruction, std::uint32_t address) const {
    if (Supports(instruction)) return;
    throw std::runtime_error("Unsupported or unvalidated host instruction at PE RVA " + std::to_string(address) + ": " + ZydisMnemonicGetString(instruction.native.mnemonic) + " (" + ZydisISASetGetString(instruction.native.meta.isa_set) + ")");
}
