#include "Fixtures.hpp"
#include <anybox/graphics/container/Dxil.hpp>
#include <algorithm>
#include <array>
#include <utility>

void CheckDxil() {
    using namespace Fixtures;
    using namespace AnyBox::Graphics::Container;
    auto program = MakeProgram();
    DxilView view(program);
    Require(view.GetProgramVersion() == 0x50065 && view.GetDxilVersion() == 0x105);
    Require(!view.HasEmbeddedData() && view.GetBitcode().size() == 8 && !view.FindEmbeddedPart(7));
    auto data = MakeXboxProgram();
    DxilView xbox(data);
    Require(xbox.HasEmbeddedData() && xbox.GetEmbeddedPartCount() == 3);
    auto nested = xbox.FindEmbeddedPart(7);
    Require(nested && std::ranges::equal(nested->data, program));
    DxilView inner(nested->data);
    Require(std::ranges::equal(inner.GetBitcode(), xbox.GetBitcode()));
    Require(inner.GetBitcode().data() == xbox.GetBitcode().data());
    Require(xbox.FindEmbeddedPart(99)->data.size() == 4 && !xbox.FindEmbeddedPart(11));
    CompilerOptionsView options(xbox.FindEmbeddedPart(10)->data);
    Require(options.GetCount() == 2 && options.FindOption(627)->value == "1" && options.FindOption(677)->value == "0");
    Reject<std::out_of_range>([&] { xbox.GetEmbeddedPart(3); });

    for (std::size_t size = 0; size < data.size(); ++size) Reject<std::invalid_argument>([&] { DxilView invalid{std::span(data).first(size)}; });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 9>{{{4, 0}, {8, 0}, {16, UINT32_MAX}, {20, UINT32_MAX}, {28, UINT32_MAX}, {32, UINT32_MAX}, {40, 0}, {52, 72}, {56, UINT32_MAX}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { DxilView rejected(invalid); });
    }
    auto duplicate = data;
    Set(duplicate, 60, 7);
    DxilView ambiguous(duplicate);
    Reject<std::invalid_argument>([&] { ambiguous.FindEmbeddedPart(7); });
}
