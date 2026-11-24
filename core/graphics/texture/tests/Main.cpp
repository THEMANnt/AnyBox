#include <anybox/texture/Resource.hpp>
#include <iostream>
#include <stdexcept>

namespace {
template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const TException&) { return; }
    throw std::runtime_error("Invalid texture GPU address or view was accepted");
}
}

int main() {
    try {
        using namespace AnyBox;
        Vulkan::Context context(0);
        Texture::AddressSpace addresses;
        std::uint64_t releasedAddress = 0;
        {
            Vulkan::ImageDescription image;
            image.format = VK_FORMAT_R8G8B8A8_UNORM;
            image.extent = {256, 256, 1};
            image.mipLevels = 9;
            image.arrayLayers = 6;
            image.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            Descriptor::Texture2DDescription description{256, 256, 6, 9, 56, 27};
            Texture::Resource resource(addresses, context, image, description, 2359296);
            releasedAddress = resource.GetAddress();
            if (&addresses.Resolve(releasedAddress) != &resource) throw std::runtime_error("Texture GPU address resolved to another resource");
            MEMORY_BASIC_INFORMATION memory{};
            if (VirtualQuery(reinterpret_cast<void*>(releasedAddress), &memory, sizeof(memory)) != sizeof(memory) || memory.State != MEM_RESERVE) throw std::runtime_error("Texture GPU address reservation committed CPU memory");
            Descriptor::TextureViewDescription view;
            view.address = releasedAddress;
            view.firstMip = 2;
            view.firstLayer = 1;
            auto words = Descriptor::EncodeTexture2D(description, view);
            auto handle = resource.GetView(words);
            auto copied = words;
            if (!handle || resource.GetView(copied) != handle) throw std::runtime_error("Copied texture descriptor did not resolve to its cached GPU view");
            auto storage = Descriptor::EncodeStorageTexture2D(description, {releasedAddress, 2, 1, 1});
            auto storageHandle = resource.GetStorageView(storage);
            if (!storageHandle || resource.GetStorageView(storage) != storageHandle) throw std::runtime_error("Storage texture descriptor did not resolve to its cached GPU view");
            Reject<std::invalid_argument>([&] { resource.GetView(storage); });
            Reject<std::invalid_argument>([&] { resource.GetStorageView(words); });
            auto invalid = words;
            invalid[0] += 1;
            Reject<std::invalid_argument>([&] { resource.GetView(invalid); });
            invalid = words;
            invalid[7] = 1;
            Reject<std::runtime_error>([&] { resource.GetView(invalid); });
            Reject<std::out_of_range>([&] { addresses.Resolve(releasedAddress + 65536); });
        }
        Reject<std::out_of_range>([&] { addresses.Resolve(releasedAddress); });
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
