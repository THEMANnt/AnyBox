#include "Queue.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) { throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!Queue method " + std::to_string(TSlot) + " is not implemented"); }

template<std::size_t... TSlots>
AnyBox::Graphics::ObjectTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ObjectTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&Queue::QueryInterface);
    result[1] = MethodAddress(&Queue::AddRef);
    result[2] = MethodAddress(&Queue::Release);
    result[3] = MethodAddress(&Queue::GetPrivateData);
    result[4] = MethodAddress(&Queue::SetPrivateData);
    result[5] = MethodAddress(&Queue::SetPrivateDataInterface);
    result[6] = MethodAddress(&Queue::SetName);
    result[7] = MethodAddress(&Queue::GetDevice);
    result[10] = MethodAddress(&Queue::ExecuteCommandLists);
    result[11] = MethodAddress(&Queue::SetMarker);
    result[12] = MethodAddress(&Queue::BeginEvent);
    result[13] = MethodAddress(&Queue::EndEvent);
    result[14] = MethodAddress(&Queue::Signal);
    result[15] = MethodAddress(&Queue::Wait);
    result[16] = MethodAddress(&Queue::GetTimestampFrequency);
    result[17] = MethodAddress(&Queue::GetClockCalibration);
    result[18] = MethodAddress(&Queue::GetDescription);
    return result;
}
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetQueueTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
    return table;
}
