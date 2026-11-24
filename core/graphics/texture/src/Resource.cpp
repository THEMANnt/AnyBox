#include <anybox/texture/Resource.hpp>
#include <anybox/platform/WindowsError.hpp>
#include <exception>
#include <limits>

AnyBox::Texture::Resource::Resource(AddressSpace& addresses, Vulkan::Context& context, const Vulkan::ImageDescription& value, const Descriptor::Texture2DDescription& description, std::uint64_t size) : addresses(addresses), image(context, value), description(description) {
    if (size == 0 || size > std::numeric_limits<std::size_t>::max()) throw std::invalid_argument("Texture GPU address reservation size is invalid");
    reservation = VirtualAlloc(nullptr, static_cast<std::size_t>(size), MEM_RESERVE, PAGE_NOACCESS);
    if (!reservation) Windows::ThrowLastError("Reserve texture GPU address range");
    try {
        initialize(size);
    } catch (...) {
        if (!VirtualFree(reservation, 0, MEM_RELEASE)) std::terminate();
        throw;
    }
}

AnyBox::Texture::Resource::Resource(AddressSpace& addresses, Vulkan::Context& context, const Vulkan::ImageDescription& value, const Descriptor::Texture2DDescription& description, std::uint64_t address, std::uint64_t size) : addresses(addresses), image(context, value), description(description), reservation(reinterpret_cast<void*>(static_cast<std::uintptr_t>(address))), ownsReservation(false) {
    initialize(size);
}

void AnyBox::Texture::Resource::initialize(std::uint64_t size) {
    Descriptor::EncodeTexture2D(description, {});
    const auto& value = image.GetDescription();
    if (value.type != VK_IMAGE_TYPE_2D || value.extent.width != description.width || value.extent.height != description.height || value.arrayLayers != description.layers || value.mipLevels != description.mipLevels || value.samples != VK_SAMPLE_COUNT_1_BIT) throw std::invalid_argument("Texture GPU image does not match its guest description");
    auto address = GetAddress();
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() || address == 0 || address > 0xFFFFFFFFFFFFULL || size - 1 > 0xFFFFFFFFFFFFULL - address || address % 65536 != 0) throw std::invalid_argument("Texture GPU address range cannot be represented");
    addresses.insert(address, *this);
}

AnyBox::Texture::Resource::~Resource() {
    addresses.remove(GetAddress());
    if (ownsReservation && !VirtualFree(reservation, 0, MEM_RELEASE)) std::terminate();
}
