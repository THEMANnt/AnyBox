#include <XCRDAPI.hpp>
#include "factory/Factory.hpp"

std::int32_t DStorageGetFactory(const AnyBox::Guid& interfaceId, void** factory) noexcept(false) {
    return AnyBox::Storage::GetFactory().QueryInterface(interfaceId, factory);
}
