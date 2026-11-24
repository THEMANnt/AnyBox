#include "Fence.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) { throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!Fence method " + std::to_string(TSlot) + " is not implemented"); }

template<std::size_t... TSlots>
AnyBox::Graphics::ObjectTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ObjectTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&Fence::QueryInterface);
    result[1] = MethodAddress(&Fence::AddRef);
    result[2] = MethodAddress(&Fence::Release);
    result[3] = MethodAddress(&Fence::GetPrivateData);
    result[4] = MethodAddress(&Fence::SetPrivateData);
    result[5] = MethodAddress(&Fence::SetPrivateDataInterface);
    result[6] = MethodAddress(&Fence::SetName);
    result[7] = MethodAddress(&Fence::GetDevice);
    result[8] = MethodAddress(&Fence::GetCompletedValue);
    result[9] = MethodAddress(&Fence::SetEventOnCompletion);
    result[10] = MethodAddress(&Fence::Signal);
    result[11] = MethodAddress(&Fence::GetCreationFlags);
    return result;
}
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetFenceTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
    return table;
}
