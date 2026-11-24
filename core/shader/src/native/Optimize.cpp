#include "Optimize.hpp"
#include "FirstLane.hpp"
#include <Optimization/ConstantFolder.hpp>
#include <Optimization/DeadCodeEliminator.hpp>
#include <Optimization/ReadLaneEliminator.hpp>
#include <Optimization/MaskedSelectEliminator.hpp>
#include <Optimization/VectorWindowIndexNarrower.hpp>
#include <Optimization/VectorWindowStoreEliminator.hpp>
#include <Optimization/DenormalFlushEliminator.hpp>
#include <Optimization/GotoPhiRefiner.hpp>

namespace {
void Simplify(ShaderRecompiler::IrProgram& program) {
    ShaderRecompiler::ConstantFolder{}.Fold(program);
    ShaderRecompiler::ResolveControlFlowIdentities(program);
    ShaderRecompiler::DeadCodeEliminator{}.RemoveIdentities(program);
    ShaderRecompiler::DeadCodeEliminator{}.Eliminate(program);
}
}

void AnyBox::Shader::OptimizeNativeProgram(ShaderRecompiler::IrProgram& program, std::uint32_t waveSize) {
    using namespace ShaderRecompiler;
    Simplify(program);
    if (ReadLaneEliminator{}.Eliminate(program, waveSize).rewrittenReads != 0) Simplify(program);
    if (VectorWindowIndexNarrower{}.Narrow(program).narrowedLoads != 0) Simplify(program);
    if (VectorWindowStoreEliminator{}.Eliminate(program).removedStores != 0) Simplify(program);
    if (MaskedSelectEliminator{}.Eliminate(program).removedSelects != 0) Simplify(program);
    if (DenormalFlushEliminator{}.Eliminate(program).removedFlushes != 0) Simplify(program);
    DeadCodeEliminator{}.EliminateUnobservedPhis(program);
    DeadCodeEliminator{}.Eliminate(program);
    if (GotoPhiRefiner{}.Refine(program).replacedOperands != 0) Simplify(program);
    if (SimplifyFirstLaneGuards(program) != 0) Simplify(program);
    ValidateProgram(program, true);
}
