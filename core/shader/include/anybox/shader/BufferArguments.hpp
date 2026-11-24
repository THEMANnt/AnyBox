#pragma once

#include "Resources.hpp"

namespace AnyBox::Shader {
struct BufferArgumentRange {
    BufferBinding binding;
    std::uint32_t firstUserRegister;
};

class BufferArgumentPlan {
public:
    static constexpr std::uint32_t WordsPerBuffer = 4;
    static constexpr std::uint32_t UserRegisterCapacity = 128;
    explicit BufferArgumentPlan(std::span<const BufferBinding> bindings, std::uint32_t firstUserRegister = 0);
    std::span<const BufferArgumentRange> GetRanges() const;
    std::uint32_t GetUserRegisterCount() const;
    const BufferArgumentRange& Find(const BufferBinding& binding) const;

private:
    std::vector<BufferArgumentRange> ranges;
    std::uint32_t userRegisterCount;
};
}
