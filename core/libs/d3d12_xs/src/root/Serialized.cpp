#include "Serialized.hpp"
#include <anybox/graphics/container/View.hpp>
#include <stdexcept>

namespace {
std::uint32_t Read(std::span<const std::byte> data, std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4; ++index) value |= std::to_integer<std::uint32_t>(data[offset + index]) << (index * 8);
    return value;
}
}

std::size_t AnyBox::Graphics::LocateSerializedRootFlags(std::span<const std::byte> data) {
    Container::DxbcView container(data);
    auto root = container.FindPart(0x30535452);
    if (!root || root->data.size() < 24) throw std::invalid_argument("Root signature container has no valid root signature part");
    auto version = Read(root->data, 0);
    if (version != 1 && version != 2) throw std::invalid_argument("Unsupported Xbox root signature part version");
    return static_cast<std::size_t>(root->data.data() - data.data()) + 20;
}

std::uint32_t AnyBox::Graphics::ReadSerializedRootFlags(std::span<const std::byte> data) {
    return Read(data, LocateSerializedRootFlags(data));
}
