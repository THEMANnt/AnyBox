#include "Reader.hpp"
#include <anybox/graphics/container/View.hpp>

AnyBox::Graphics::Container::RootSignatureLayout AnyBox::Graphics::Container::ReadRootSignatureLayout(std::span<const std::byte> data) {
    DxbcView container(data);
    std::optional<Part> root;
    for (unsigned index = 0; index < container.GetPartCount(); ++index) {
        auto part = container.GetPart(index);
        if (part.tag != 0x30535452) continue;
        if (root) throw std::invalid_argument("Root signature part is duplicated");
        root = part;
    }
    if (!root) throw std::invalid_argument("DXBC container has no root signature part");
    Root::Reader reader(root->data);
    reader.Check(0, 1, 24);
    auto version = reader.Word(0);
    if (version != 1 && version != 2) throw std::runtime_error("Root signature version is not implemented");
    auto count = reader.Word(4);
    auto base = reader.Word(8);
    auto samplers = reader.Word(12);
    reader.Check(base, count, 12);
    reader.Check(reader.Word(16), samplers, 52);
    RootSignatureLayout layout{version, reader.Word(20), samplers, {}};
    if ((layout.flags & ~0x83000FFFU) != 0) throw std::invalid_argument("Root signature flags are invalid");
    for (std::uint32_t index = 0; index < count; ++index) layout.parameters.push_back(reader.Parameter(static_cast<std::size_t>(base) + index * 12, version == 1));
    return layout;
}
