#include <SpirvBackend/SpirvMemory/SpirvBufferAccess.hpp>
#include <spirv/unified1/spirv.hpp>

bool ShaderRecompiler::ScratchPoolActive(const SpirvEmitterState&) { return false; }

std::uint32_t ShaderRecompiler::ScratchPoolSlotDwords(const SpirvEmitterState&) {
    throw std::logic_error("AnyBox uses invocation-local scratch storage");
}

void ShaderRecompiler::EmitScratchPoolAcquire(SpirvEmitterState& state) {
    if (!state.requirements.functionScratch) return;
    if (state.program.Info().scratchDwords == 0) throw std::logic_error("Invocation-local scratch storage has zero size");
    for (std::uint32_t half = 0; half < state.laneCount; ++half) {
        if (state.scratchVariable.at(half) == 0) throw std::logic_error("Invocation-local scratch storage was not declared");
    }
}

std::uint32_t ShaderRecompiler::EmitScratchPoolElementPointer(SpirvEmitterState&, std::uint32_t) {
    throw std::logic_error("AnyBox cannot access an external scratch pool");
}

void ShaderRecompiler::EmitReturnReleasingScratch(SpirvEmitterState& state) {
    if (state.scratchPoolVariable != 0) throw std::logic_error("Invocation-local scratch unexpectedly references an external pool");
    state.module.AddFunction(spv::OpReturn);
}

void ShaderRecompiler::VerifyScratchPoolReturns(const SpirvEmitterState& state, const std::vector<std::uint32_t>& words) {
    if (words.size() < 5 || words.front() != 0x07230203) throw std::logic_error("Invocation-local scratch compilation produced an invalid SPIR-V header");
    if (state.scratchPoolVariable != 0 || state.scratchPoolReturns != 0) throw std::logic_error("Invocation-local scratch unexpectedly emitted external pool management");
}
