#include "Reader.hpp"

using namespace AnyBox::Graphics::Container;
using AnyBox::Graphics::Container::Root::Reader;

std::vector<RootRange> Reader::ranges(std::size_t offset, bool version1) const {
    auto count = Word(offset);
    auto base = Word(offset + 4);
    auto stride = version1 ? 20u : 24u;
    Check(base, count, stride);
    std::vector<RootRange> output;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto position = static_cast<std::size_t>(base) + index * stride;
        auto type = Word(position);
        if (type > 3) throw std::invalid_argument("Root descriptor range type is invalid");
        auto flags = version1 ? 0 : Word(position + 16);
        if ((flags & ~0x1000FU) != 0) throw std::invalid_argument("Root descriptor range flags are invalid");
        auto descriptors = Word(position + 4);
        if (descriptors == 0) throw std::invalid_argument("Root descriptor range is empty");
        output.push_back({static_cast<RootRangeKind>(type), descriptors, Word(position + 8), Word(position + 12), flags, Word(position + (version1 ? 16 : 20))});
    }
    return output;
}

RootParameter Reader::Parameter(std::size_t offset, bool version1) const {
    auto type = Word(offset);
    auto visibility = Word(offset + 4);
    auto payload = Word(offset + 8);
    if (type > 4 || visibility > 7) throw std::invalid_argument("Root parameter type or visibility is invalid");
    RootParameter result{};
    result.kind = static_cast<RootParameterKind>(type);
    result.visibility = visibility;
    if (result.kind == RootParameterKind::Table) {
        result.ranges = ranges(payload, version1);
        if (result.ranges.empty()) throw std::invalid_argument("Root descriptor table is empty");
    } else {
        Check(payload, 1, result.kind == RootParameterKind::Constants || !version1 ? 12 : 8);
        result.shaderRegister = Word(payload);
        result.space = Word(payload + 4);
        if (result.kind == RootParameterKind::Constants) {
            result.wordCount = Word(payload + 8);
            if (result.wordCount == 0) throw std::invalid_argument("Root constant range is empty");
        } else if (!version1) {
            result.flags = Word(payload + 8);
            if ((result.flags & ~0xEU) != 0) throw std::invalid_argument("Root descriptor flags are invalid");
        }
    }
    return result;
}
