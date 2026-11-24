#include "Resource.hpp"
#include "../object/Table.hpp"

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetResourceTable() {
    static const auto table = [] {
        auto value = MakeObjectTable<Resource>(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
        value[8] = MethodAddress(&Resource::Map);
        value[9] = MethodAddress(&Resource::Unmap);
        value[10] = MethodAddress(&Resource::GetDescription);
        value[11] = MethodAddress(&Resource::GetGPUVirtualAddress);
        value[14] = MethodAddress(&Resource::GetHeapProperties);
        value[16] = MethodAddress(&Resource::GetDescription1);
        return value;
    }();
    return table;
}
