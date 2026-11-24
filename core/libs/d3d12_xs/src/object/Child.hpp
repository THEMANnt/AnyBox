#pragma once

#include "Lifetime.hpp"
#include "Table.hpp"

namespace AnyBox::Graphics {
template<class TNative, Guid TInterfaceId, bool TPageable = false>
class DeviceChild : public ObjectLifetime<DeviceChild<TNative, TInterfaceId, TPageable>> {
    using Self = DeviceChild<TNative, TInterfaceId, TPageable>;

public:
    DeviceChild(TNative* host, Device* device) : vtable(&GetObjectTable<Self>()), host(host), device(device) { Device::AddRef(device); }
    ~DeviceChild() {
        host->Release();
        Device::Release(device);
    }
    TNative* GetHost() const noexcept { return host; }
    Device* GetParent() const noexcept { return device; }
    static constexpr std::size_t GetVtableOffset() { return offsetof(Self, vtable); }
    static bool Supports(const Guid& interfaceId) noexcept { return interfaceId == TInterfaceId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || (TPageable && interfaceId == PageableId); }
    static Self* From(void* object) {
        if (!object) throw std::invalid_argument("Graphics object is null");
        const ObjectTable* table;
        std::memcpy(&table, object, sizeof(table));
        if (table != &GetObjectTable<Self>()) throw std::invalid_argument("Foreign graphics object interface is unsupported");
        return static_cast<Self*>(object);
    }

private:
    friend class ObjectLifetime<Self>;
    const ObjectTable* vtable;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::atomic<std::uint32_t> references{1};
    TNative* host;
    Device* device;
};
}
