#include <anybox/graphics/container/Native.hpp>
#include <anybox/graphics/container/Dxil.hpp>
#include <stdexcept>

std::vector<std::vector<std::byte>> AnyBox::Graphics::Container::UnpackNativePrograms(std::span<const std::byte> data) {
    DxbcView container(data);
    auto part = container.FindPart(0x4C495844);
    if (!part) throw std::invalid_argument("Xbox native shader container has no program header");
    DxilView program(part->data);
    std::vector<std::vector<std::byte>> output;
    for (std::uint32_t index = 0; index < program.GetEmbeddedPartCount(); ++index) {
        auto entry = program.GetEmbeddedPart(index);
        if (entry.tag == 13) output.push_back(UnpackXbch(entry.data));
    }
    if (output.empty()) throw std::invalid_argument("Xbox shader container has no native programs");
    return output;
}
