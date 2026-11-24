#include "Heap.hpp"
#include "../object/Table.hpp"

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetDescriptorHeapTable() {
    static const auto table = [] {
        auto value = MakeObjectTable<DescriptorHeap>(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
        value[8] = MethodAddress(&DescriptorHeap::GetDescription);
        value[9] = MethodAddress(&DescriptorHeap::GetCpuStart);
        value[10] = MethodAddress(&DescriptorHeap::GetGpuStart);
        return value;
    }();
    return table;
}
