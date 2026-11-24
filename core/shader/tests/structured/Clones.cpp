#include "Fixture.hpp"
#include "flow/Plan.hpp"
#include <stdexcept>

void StructuredTests::CheckClones(AnyBox::Vulkan::Context& context) {
    constexpr std::array<std::array<unsigned, 2>, 4> edges{{{1, 2}, {2, 3}, {3, 4}, {4, 5}}};
    Fixture fixture;
    auto& builder = fixture.builder;
    std::array<Fixture::IrBlock*, 6> blocks{fixture.entry};
    for (unsigned node = 1; node < blocks.size(); ++node) blocks[node] = &fixture.Block();
    std::array<Fixture::IrValue*, 4> conditions;
    for (unsigned node = 0; node < conditions.size(); ++node) conditions[node] = &builder.INotEqual(builder.BitwiseAnd(*fixture.index, builder.Constant(1U << node)), builder.Constant(0));
    for (unsigned node = 0; node < conditions.size(); ++node) fixture.Conditional(*blocks[node], *conditions[node], *blocks[edges[node][0]], *blocks[edges[node][1]]);
    fixture.Enter(*blocks[4]);
    fixture.Return(*blocks[4], builder.IAdd(*fixture.index, builder.Constant(5)));
    fixture.Enter(*blocks[5]);
    fixture.Return(*blocks[5], builder.IMul(*fixture.index, builder.Constant(3)));
    AnyBox::Shader::Dxil::Flow::Plan plan(fixture.program);
    std::vector<unsigned> counts(plan.origins.size());
    bool cloned = false;
    for (const auto& block : plan.graph.blocks) {
        if (block.instructionBegin == block.instructionEnd) continue;
        cloned |= ++counts.at(block.instructionBegin) > 1;
    }
    if (!cloned) throw std::runtime_error("Shared-return regression did not exercise block cloning");
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) {
        unsigned node = 0;
        while (node < edges.size()) node = edges[node][(index & (1U << node)) != 0 ? 0 : 1];
        expected[index] = node == 4 ? index + 5 : index * 3;
    }
    fixture.Run(context, expected);
}
