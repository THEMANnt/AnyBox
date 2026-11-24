#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyBox::Graphics::Container {
std::vector<std::byte> UnpackXbch(std::span<const std::byte> data);
std::vector<std::vector<std::byte>> UnpackNativePrograms(std::span<const std::byte> container);

class FssmView {
public:
    explicit FssmView(std::span<const std::byte> data);
    std::span<const std::byte> GetCode() const noexcept { return code; }
    std::uint32_t GetWaveSize() const noexcept { return waveSize; }
    std::uint64_t GetCodeSectionOffset() const noexcept { return codeSectionOffset; }
    std::uint64_t GetCodeSectionSize() const noexcept { return codeSectionSize; }

private:
    std::span<const std::byte> code;
    std::uint64_t codeSectionOffset;
    std::uint64_t codeSectionSize;
    std::uint32_t waveSize;
};
}
