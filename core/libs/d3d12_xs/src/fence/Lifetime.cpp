#include "Fence.hpp"
#include <stdexcept>

AnyBox::Graphics::Fence::Fence(ID3D12Fence* host, Device* device, D3D12_FENCE_FLAGS flags) : vtable(&GetFenceTable()), host(host), device(device), flags(flags) { Device::AddRef(device); }

AnyBox::Graphics::Fence::~Fence() {
    host->Release();
    Device::Release(device);
}

AnyBox::Graphics::Fence* AnyBox::Graphics::Fence::From(void* object) {
    if (!object) throw std::invalid_argument("Fence is null");
    const ObjectTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetFenceTable()) throw std::invalid_argument("Foreign fence interface is unsupported");
    return static_cast<Fence*>(object);
}

bool AnyBox::Graphics::Fence::Supports(const Guid& interfaceId) noexcept { return interfaceId == FenceId || interfaceId == Fence1Id || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }

