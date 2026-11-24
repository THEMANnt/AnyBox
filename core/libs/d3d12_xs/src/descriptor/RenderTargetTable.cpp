#include "RenderTarget.hpp"
#include <string>
#include <utility>

namespace {
std::uint32_t GetType(void*) { return 0; }

template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) {
    throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!Render target descriptor method " + std::to_string(TSlot) + " is not implemented");
}

template<std::size_t... TSlots>
AnyBox::Graphics::ObjectTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ObjectTable table{MethodAddress(&Unsupported<TSlots>)...};
    table[0] = MethodAddress(&GetType);
    return table;
}
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetRenderTargetTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
    return table;
}
