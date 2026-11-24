#include "Plan.hpp"

std::optional<AnyBox::Shader::Reductions::Plan> AnyBox::Shader::Reductions::Find(const Graph& graph, std::uint32_t result) {
    const auto& root = graph.Get(result);
    std::uint32_t left = 0;
    std::uint32_t right = 0;
    auto maximum = !graph.MinMax(root, false, left, right);
    if (!graph.MinMax(root, maximum, left, right) || !graph.UnsignedWord(root.words[1]) || root.block == 0) return std::nullopt;
    std::uint32_t lanes = 0;
    std::uint32_t source = 0;
    std::uint32_t active = 0;
    std::uint32_t scope = 0;
    auto current = result;
    for (unsigned index = 0; index < 32; ++index) {
        const auto& reduction = graph.Get(current);
        if (!graph.MinMax(reduction, maximum, left, right) || reduction.block != root.block || reduction.words[1] != root.words[1]) return std::nullopt;
        const auto& shuffle = graph.Get(right);
        if (!shuffle.Is(spv::OpGroupNonUniformShuffle, 6) || shuffle.block != root.block || shuffle.words[1] != root.words[1] || !graph.Integer(shuffle.words[3], spv::ScopeSubgroup)) return std::nullopt;
        unsigned lane = 0;
        std::uint32_t word = 0;
        if (!graph.Selector(shuffle.words[5], root.block, lane, word) || (lanes & (std::uint32_t{1} << lane)) != 0) return std::nullopt;
        if (index != 0 && (source != shuffle.words[4] || active != word)) return std::nullopt;
        source = shuffle.words[4];
        active = word;
        scope = shuffle.words[3];
        lanes |= std::uint32_t{1} << lane;
        current = left;
    }
    if (lanes != UINT32_MAX || current != source) return std::nullopt;
    return Plan{maximum ? spv::OpGroupNonUniformUMax : spv::OpGroupNonUniformUMin, scope, source};
}
