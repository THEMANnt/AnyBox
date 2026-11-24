#include "Configuration.hpp"

#include <anybox/platform/Windows.hpp>

#include <algorithm>
#include <array>
#include <string_view>

std::atomic<std::uint32_t>& AnyBox::Storage::GetDebugFlags() noexcept {
    static std::atomic<std::uint32_t> flags{0};
    return flags;
}

void AnyBox::Storage::ReportValidationFailure(const char* function) noexcept {
    auto flags = GetDebugFlags().load(std::memory_order_relaxed);
    if ((flags & 1) != 0) {
        constexpr std::string_view prefix = "AnyBox DirectStorage: validation failed in ";
        std::array<char, 512> message{};
        auto output = std::copy(prefix.begin(), prefix.end(), message.begin());
        std::string_view name(function);
        auto size = std::min(name.size(), message.size() - prefix.size() - 2);
        output = std::copy_n(name.begin(), size, output);
        *output = '\n';
        OutputDebugStringA(message.data());
    }
    if ((flags & 2) != 0) DebugBreak();
}
