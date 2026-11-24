#include "Graph.hpp"
#include <bit>

using namespace AnyBox::Shader::Reductions;

bool Graph::Selector(std::uint32_t id, std::uint32_t block, unsigned& lane, std::uint32_t& active) const {
    auto* selector = &Get(id);
    if (selector->Is(spv::OpBitwiseAnd, 5) && Integer(selector->words[4], 31)) selector = &Get(selector->words[3]);
    if (!selector->Is(spv::OpSelect, 6) || selector->block != block) return false;
    const auto& condition = Get(selector->words[3]);
    if (!condition.Is(spv::OpINotEqual, 5) || condition.block != block || !Integer(condition.words[4], 0)) return false;
    const auto& bit = Get(condition.words[3]);
    if (!bit.Is(spv::OpBitwiseAnd, 5) || bit.block != block || !ActiveWord(bit.words[3], block)) return false;
    const auto& mask = Get(bit.words[4]);
    if (!mask.Is(spv::OpConstant, 4) || !UnsignedWord(mask.words[1]) || !std::has_single_bit(mask.words[3])) return false;
    lane = static_cast<unsigned>(std::countr_zero(mask.words[3]));
    if (!Integer(selector->words[4], lane)) return false;
    auto* first = &Get(selector->words[5]);
    if (first->Is(spv::OpBitcast, 4) && UnsignedWord(first->words[1]) && first->block == block) first = &Get(first->words[3]);
    if (!Glsl(*first, GLSLstd450FindILsb, 1) || first->block != block || first->words[5] != bit.words[3]) return false;
    active = bit.words[3];
    return true;
}
