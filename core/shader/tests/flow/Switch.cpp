#include "Fixture.hpp"
#include "Fixtures.hpp"
#include <unordered_map>

namespace {
void CheckOrder() {
    FlowTests::Fixture fixture;
    auto* entry = fixture.Block();
    auto* first = fixture.Block();
    auto* second = fixture.Block();
    auto* other = fixture.Block();
    auto* selection = fixture.Emit<LLVMBC::SwitchInst>(entry, fixture.Constant(1), other, 2);
    selection->addCase(fixture.Constant(0), first);
    selection->addCase(fixture.Constant(1), second);
    for (auto* block : {first, second, other}) fixture.Emit<LLVMBC::ReturnInst>(block, nullptr);
    auto output = fixture.Lower();
    std::unordered_map<const ShaderRecompiler::IrBlock*, std::size_t> positions;
    for (auto* block : output.BlockOrder()) positions.emplace(block, positions.size());
    for (auto* block : output.BlockOrder()) {
        Fixtures::Require(output.Metadata().blockInfo.at(positions.at(block)).id == block->Id());
        if (block->Predecessors().size() == 1) Fixtures::Require(positions.at(block->Predecessors().front()) < positions.at(block));
    }
}
}

void FlowTests::CheckSwitches() {
    CheckOrder();
    for (unsigned selector = 0; selector < 16; ++selector) {
        Fixture fixture;
        auto* entry = fixture.Block();
        auto* merge = fixture.Block();
        auto* shared = fixture.Block();
        auto* selection = fixture.Emit<LLVMBC::SwitchInst>(entry, fixture.Constant(selector), merge, 13);
        for (unsigned key = 0; key < 13; ++key) selection->addCase(fixture.Constant(key), key % 2 == 0 ? merge : shared);
        fixture.Emit<LLVMBC::BranchInst>(shared, merge);
        auto* phi = fixture.Emit<LLVMBC::PHINode>(merge, LLVMBC::Type::getInt16Ty(fixture.Context()), 9);
        for (unsigned index = 0; index < 8; ++index) phi->add_incoming(fixture.Constant(100), entry);
        phi->add_incoming(fixture.Constant(200), shared);
        fixture.Emit<LLVMBC::ReturnInst>(merge, nullptr);
        auto output = fixture.Lower();
        auto expected = selector < 13 && selector % 2 != 0 ? 200u : 100u;
        Fixtures::Require(Evaluate(output) == std::vector<std::uint32_t>{expected});
    }
}
