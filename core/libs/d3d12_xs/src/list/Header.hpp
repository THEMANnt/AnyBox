#pragma once

#include "../device/Device.hpp"

namespace AnyBox::Graphics {
using ListTable = std::array<DeviceMethod, 256>;

struct GuestListHeader {
    GuestListHeader(const ListTable* table, D3D12_COMMAND_LIST_TYPE kind) : vtable(table), type(kind) {}
    const ListTable* vtable;
    std::uint32_t objectFlags = 0;
    std::atomic<std::uint32_t> references{1};
    D3D12_COMMAND_LIST_TYPE type;
    std::uint32_t reserved = 0;
    std::uint32_t* commandWrite = nullptr;
    std::uintptr_t driverAddress = 0;
    std::uint32_t* dispatchEnd = nullptr;
    std::uint32_t* commandEnd = nullptr;
    std::array<std::byte, 4096 - 56> driverState{};
};

static_assert(std::is_standard_layout_v<GuestListHeader>);
static_assert(sizeof(GuestListHeader) == 4096);
static_assert(offsetof(GuestListHeader, vtable) == 0);
static_assert(offsetof(GuestListHeader, references) == 12);
static_assert(offsetof(GuestListHeader, type) == 16);
static_assert(offsetof(GuestListHeader, commandWrite) == 24);
static_assert(offsetof(GuestListHeader, dispatchEnd) == 40);
static_assert(offsetof(GuestListHeader, commandEnd) == 48);
}
