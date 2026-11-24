#pragma once

#include "../device/Device.hpp"
#include "../object/Lifetime.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid FenceId{0x0A753DCF, 0xC4D8, 0x4B91, {0xAD, 0xF6, 0xBE, 0x5A, 0x60, 0xD9, 0x5A, 0x76}};
inline constexpr Guid Fence1Id{0x433685FE, 0xE22B, 0x4CA0, {0xA8, 0xDB, 0xB5, 0xB4, 0xF4, 0xDD, 0x0E, 0x4A}};
const ObjectTable& GetFenceTable();

class Fence : public ObjectLifetime<Fence> {
public:
    Fence(ID3D12Fence* host, Device* device, D3D12_FENCE_FLAGS flags);
    ~Fence();
    static constexpr std::size_t GetVtableOffset() { return offsetof(Fence, vtable); }
    ID3D12Fence* GetHost() const noexcept { return host; }
    static Fence* From(void* object);
    static bool Supports(const Guid& interfaceId) noexcept;
    static std::uint64_t GetCompletedValue(Fence* self);
    static HRESULT SetEventOnCompletion(Fence* self, std::uint64_t value, HANDLE event);
    static HRESULT Signal(Fence* self, std::uint64_t value);
    static D3D12_FENCE_FLAGS GetCreationFlags(Fence* self);

private:
    friend class ObjectLifetime<Fence>;
    const ObjectTable* vtable;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::atomic<std::uint32_t> references{1};
    ID3D12Fence* host;
    Device* device;
    D3D12_FENCE_FLAGS flags;
};

static_assert(std::is_standard_layout_v<Fence>);
static_assert(Fence::GetVtableOffset() == 0);
}
