#include <anybox/relinker/Image.hpp>

void AnyBox::Relinker::Image::appendGuardTable(std::vector<std::uint32_t>& entries, std::size_t offset, std::size_t stride) const {
    auto address = Read<std::uint64_t>(GetBytes(), offset);
    auto count = Read<std::uint64_t>(GetBytes(), offset + 8);
    if (count == 0) return;
    if (address < base || address - base > UINT32_MAX || count > UINT32_MAX / stride) throw std::runtime_error("PE guard function table bounds are invalid");
    auto table = FileOffset(static_cast<std::uint32_t>(address - base), static_cast<std::size_t>(count) * stride);
    for (std::size_t index = 0; index < count; ++index) {
        auto target = Read<std::uint32_t>(GetBytes(), table + index * stride);
        if (!IsExecutable(target)) throw std::runtime_error("PE guard function target is not executable");
        entries.push_back(target);
    }
}

void AnyBox::Relinker::Image::appendLoaderEntries(std::vector<std::uint32_t>& entries) const {
    auto tls = directories[9];
    if (tls.size != 0) {
        if (tls.size < 40) throw std::runtime_error("PE TLS directory is truncated");
        auto callbacks = Read<std::uint64_t>(GetBytes(), FileOffset(tls.address, 40) + 24);
        if (callbacks != 0) {
            if (callbacks < base || callbacks - base > UINT32_MAX) throw std::runtime_error("PE TLS callback array lies outside the image");
            auto address = callbacks - base;
            for (;;) {
                if (address > UINT32_MAX) throw std::runtime_error("PE TLS callback array address overflows");
                auto target = Read<std::uint64_t>(GetBytes(), FileOffset(static_cast<std::uint32_t>(address), 8));
                if (target == 0) break;
                if (target < base || target - base > UINT32_MAX || !IsExecutable(static_cast<std::uint32_t>(target - base))) throw std::runtime_error("PE TLS callback target is not executable");
                entries.push_back(static_cast<std::uint32_t>(target - base));
                address += 8;
            }
        }
    }

    auto config = directories[10];
    if (config.size == 0) return;
    if (config.size < 4) throw std::runtime_error("PE load configuration directory is truncated");
    auto offset = FileOffset(config.address, config.size);
    auto size = Read<std::uint32_t>(GetBytes(), offset);
    if (size < 4 || size > config.size) throw std::runtime_error("PE load configuration size is invalid");
    if (size >= 148) appendGuardTable(entries, offset + 128, 4 + (Read<std::uint32_t>(GetBytes(), offset + 144) >> 28));
    if (size >= 192) appendGuardTable(entries, offset + 176, 4);
    if (size >= 280) appendGuardTable(entries, offset + 264, 4);
}
