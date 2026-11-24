#include "Fixtures.hpp"
#include <anybox/graphics/container/RootSignature.hpp>
#include <anybox/graphics/container/View.hpp>
#include <array>

namespace {
std::vector<std::byte> Make(unsigned version) {
    std::vector<std::uint32_t> words{version, 3, 24, 0, 0, 0x3000000, 3, 0, 60, 1, 0, 0, 0, 0, 0};
    words.insert(words.end(), {7, 2});
    if (version == 2) words.push_back(4);
    words[11] = static_cast<std::uint32_t>(words.size() * 4);
    words.insert(words.end(), {5, 3, 5});
    words[14] = static_cast<std::uint32_t>(words.size() * 4);
    words.push_back(2);
    words.push_back(static_cast<std::uint32_t>((words.size() + 1) * 4));
    words.insert(words.end(), {0, 4, 3, 6});
    if (version == 2) words.push_back(2);
    words.push_back(UINT32_MAX);
    words.insert(words.end(), {1, 2, 9, 2});
    if (version == 2) words.push_back(0);
    words.push_back(12);
    words[4] = static_cast<std::uint32_t>(words.size() * 4);
    std::vector<std::byte> data(words.size() * 4);
    for (std::size_t index = 0; index < words.size(); ++index) Fixtures::Set(data, index * 4, words[index]);
    return data;
}

auto Read(std::span<const std::byte> data) {
    using namespace AnyBox::Graphics::Container;
    return ReadRootSignatureLayout(BuildDxbc(std::array{Part{0x30535452, data}}));
}
}

void CheckRootSignatureLayout() {
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    for (unsigned version : {1u, 2u}) {
        auto data = Make(version);
        auto root = Read(data);
        Require(root.version == version && root.flags == 0x3000000 && root.staticSamplerCount == 0 && root.parameters.size() == 3);
        const auto& descriptor = root.parameters[0];
        Require(descriptor.kind == RootParameterKind::ShaderResource && descriptor.shaderRegister == 7 && descriptor.space == 2 && descriptor.flags == (version == 1 ? 0u : 4u));
        const auto& constants = root.parameters[1];
        Require(constants.kind == RootParameterKind::Constants && constants.shaderRegister == 5 && constants.space == 3 && constants.wordCount == 5);
        const auto& table = root.parameters[2];
        Require(table.kind == RootParameterKind::Table && table.ranges.size() == 2);
        Require(table.ranges[0].kind == RootRangeKind::ShaderResource && table.ranges[0].count == 4 && table.ranges[0].firstRegister == 3 && table.ranges[0].space == 6 && table.ranges[0].tableOffset == UINT32_MAX);
        Require(table.ranges[1].kind == RootRangeKind::UnorderedAccess && table.ranges[1].tableOffset == 12);
        for (std::size_t size = 0; size < data.size(); ++size) Reject<std::invalid_argument>([&] { Read(std::span(data).first(size)); });
        for (auto [offset, value] : std::array<std::array<std::uint32_t, 2>, 7>{{{4, UINT32_MAX}, {8, 25}, {12, 1}, {20, 0x40000000}, {24, 5}, {28, 8}, {32, UINT32_MAX}}}) {
            auto invalid = data;
            Set(invalid, offset, value);
            Reject<std::invalid_argument>([&] { Read(invalid); });
        }
    }
    auto data = Make(2);
    Set(data, 0, 3);
    Reject<std::runtime_error>([&] { Read(data); });
    auto valid = Make(2);
    auto duplicate = BuildDxbc(std::array{Part{0x30535452, valid}, Part{0x30535452, valid}});
    Reject<std::invalid_argument>([&] { ReadRootSignatureLayout(duplicate); });
    Reject<std::invalid_argument>([] { ReadRootSignatureLayout(BuildDxbc({})); });
}
