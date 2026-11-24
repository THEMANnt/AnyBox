#pragma once

#include "AddressSpace.hpp"
#include <anybox/descriptor/Texture.hpp>
#include <anybox/vulkan/ImageView.hpp>
#include <memory>
#include <map>
#include <mutex>

namespace AnyBox::Texture {
class Resource {
public:
    Resource(AddressSpace& addresses, Vulkan::Context& context, const Vulkan::ImageDescription& image, const Descriptor::Texture2DDescription& description, std::uint64_t size);
    Resource(AddressSpace& addresses, Vulkan::Context& context, const Vulkan::ImageDescription& image, const Descriptor::Texture2DDescription& description, std::uint64_t address, std::uint64_t size);
    ~Resource();
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Vulkan::Image& GetImage() noexcept { return image; }
    std::uint64_t GetAddress() const noexcept { return reinterpret_cast<std::uintptr_t>(reservation); }
    const Descriptor::Texture2DDescription& GetDescription() const noexcept { return description; }
    VkImageView GetView(const Descriptor::ImageWords& words);
    VkImageView GetStorageView(const Descriptor::ImageWords& words);

private:
    VkImageView getView(const Descriptor::ImageWords& words, bool storage);
    void initialize(std::uint64_t size);
    AddressSpace& addresses;
    Vulkan::Image image;
    Descriptor::Texture2DDescription description;
    void* reservation = nullptr;
    bool ownsReservation = true;
    std::mutex viewMutex;
    std::map<Descriptor::ImageWords, std::unique_ptr<Vulkan::ImageView>> views;
};
}
