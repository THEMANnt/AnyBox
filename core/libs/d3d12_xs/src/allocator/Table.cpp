#include "Allocator.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) { throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!Allocator method " + std::to_string(TSlot) + " is not implemented"); }

template<std::size_t... TSlots>
AnyBox::Graphics::ObjectTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ObjectTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&Allocator::QueryInterface);
    result[1] = MethodAddress(&Allocator::AddRef);
    result[2] = MethodAddress(&Allocator::Release);
    result[3] = MethodAddress(&Allocator::GetPrivateData);
    result[4] = MethodAddress(&Allocator::SetPrivateData);
    result[5] = MethodAddress(&Allocator::SetPrivateDataInterface);
    result[6] = MethodAddress(&Allocator::SetName);
    result[7] = MethodAddress(&Allocator::GetDevice);
    result[8] = MethodAddress(&Allocator::Reset);
    return result;
}
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetAllocatorTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
    return table;
}
