#pragma once

#include "PlacedTransfer.hpp"
#include <bit>

namespace AnyBox::Graphics::Tests {
inline GUID MetadataGuid(const Guid& key) { return std::bit_cast<GUID>(key); }
HRESULT ReadResourceData(Resource* resource, const Guid& key, std::uint32_t* size, void* data);
void CheckResourceMetadata(Device* device, Resource* resource);
void CheckResourceInterfaces(Device* device, Resource* resource);
}
