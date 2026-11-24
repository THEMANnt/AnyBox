#include <anybox/codecs/XmaFrameAssembler.hpp>
#include <algorithm>
#include <cstring>

void AnyBox::Codecs::XmaFrameAssembler::append(std::span<const std::byte> source, std::uint32_t offset, std::uint32_t count) {
    while (count != 0) {
        if (savedBits % 8 == 0 && offset % 8 == 0 && count >= 8) {
            auto bytes = count / 8;
            std::memcpy(data.data() + savedBits / 8, source.data() + offset / 8, bytes);
            savedBits += bytes * 8;
            offset += bytes * 8;
            count %= 8;
            continue;
        }
        auto chunk = std::min({count, 8 - savedBits % 8, 8 - offset % 8});
        auto mask = (1u << chunk) - 1;
        auto value = (std::to_integer<std::uint32_t>(source[offset / 8]) >> (8 - offset % 8 - chunk)) & mask;
        auto shift = 8 - savedBits % 8 - chunk;
        auto previous = std::to_integer<std::uint32_t>(data[savedBits / 8]);
        data[savedBits / 8] = static_cast<std::byte>((previous & ~(mask << shift)) | (value << shift));
        savedBits += chunk;
        offset += chunk;
        count -= chunk;
    }
}

void AnyBox::Codecs::XmaFrameAssembler::Reset() noexcept {
    savedBits = 0;
    frameBits = 0;
}

bool AnyBox::Codecs::XmaFrameAssembler::HasPartialFrame() const noexcept { return savedBits != 0; }
