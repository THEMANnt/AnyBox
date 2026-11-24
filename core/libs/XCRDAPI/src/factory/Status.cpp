#include "Factory.hpp"
#include "../status/StatusArray.hpp"

#include <stdexcept>

std::int32_t AnyBox::Storage::Factory::CreateStatusArray(std::uint32_t capacity, const char* name, const Guid& interfaceId, void** statusArray) {
    if (!statusArray) throw std::invalid_argument("DirectStorage status array output is null");
    if (interfaceId != StatusArrayId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage status interface");
    auto* array = new StatusArray(capacity, name);
    *statusArray = static_cast<IStorageStatusArray*>(array);
    return 0;
}
