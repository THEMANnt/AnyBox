#include <anybox/vulkan/Image.hpp>
#include <utility>

AnyBox::Vulkan::Image::Image(Context& context, const ImageDescription& description) : context(context), description(description) {
    try {
        create();
        allocate();
    } catch (...) { release(); throw; }
}

AnyBox::Vulkan::Image::Image(Image&& other) noexcept : context(other.context), description(other.description), image(std::exchange(other.image, VK_NULL_HANDLE)), memory(std::exchange(other.memory, VK_NULL_HANDLE)), allocationSize(std::exchange(other.allocationSize, 0)) {}

AnyBox::Vulkan::Image::~Image() { release(); }

void AnyBox::Vulkan::Image::release() noexcept {
    const auto& functions = context.GetFunctions();
    if (image) functions.destroyImage(context.GetDevice(), image, nullptr);
    if (memory) functions.freeMemory(context.GetDevice(), memory, nullptr);
}
