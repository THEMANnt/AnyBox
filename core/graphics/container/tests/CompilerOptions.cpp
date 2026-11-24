#include "Fixtures.hpp"
#include <anybox/graphics/container/Dxil.hpp>
#include <array>
#include <utility>

void CheckCompilerOptions() {
    using namespace Fixtures;
    using namespace AnyBox::Graphics::Container;
    auto data = MakeOptions();
    CompilerOptionsView view(data);
    Require(view.GetCount() == 2 && view.GetOption(0).key == 627 && view.GetOption(1).value == "0");
    Require(!view.FindOption(1));
    Reject<std::out_of_range>([&] { view.GetOption(2); });
    for (std::size_t size = 0; size < data.size(); ++size) Reject<std::invalid_argument>([&] { CompilerOptionsView rejected{std::span(data).first(size)}; });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 4>{{{0, 0}, {4, UINT32_MAX}, {12, UINT32_MAX}, {21, UINT32_MAX}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { CompilerOptionsView rejected(invalid); });
    }
    auto duplicate = data;
    Set(duplicate, 17, 627);
    CompilerOptionsView ambiguous(duplicate);
    Reject<std::invalid_argument>([&] { ambiguous.FindOption(627); });
    auto trailing = data;
    trailing.push_back(std::byte{});
    Reject<std::invalid_argument>([&] { CompilerOptionsView rejected(trailing); });
}
