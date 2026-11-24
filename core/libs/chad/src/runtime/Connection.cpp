#include "Connection.hpp"

#include <algorithm>
#include <stdexcept>

AnyBox::Dsp::Connection::Connection(void* memory, std::uint32_t size, std::uint32_t impulseResponseSeconds, std::uint32_t maximumStreamCount) : base(reinterpret_cast<std::uintptr_t>(memory)), size(size), impulseResponseSeconds(impulseResponseSeconds), maximumStreamCount(maximumStreamCount), stateCapacity(0) {
    auto samples = static_cast<std::size_t>(impulseResponseSeconds) * 48000;
    for (std::size_t frames : {512U, 1024U}) {
        auto streams = std::min(samples / (9 * frames), std::size_t{256});
        auto overhead = frames == 512 ? 155648 : 278528;
        auto bytes = streams * overhead + 16 * (samples - samples % frames);
        stateCapacity = std::max(stateCapacity, (bytes + 8191) & ~std::size_t{8191});
    }
}

void AnyBox::Dsp::Connection::ValidateBuffer(const void* address, std::size_t bufferSize) const {
    auto value = reinterpret_cast<std::uintptr_t>(address);
    if (value < base || value - base > size || bufferSize > size - (value - base)) throw std::invalid_argument("DSP buffer is outside connection storage");
    if (value % alignof(float) != 0) throw std::invalid_argument("DSP buffer is misaligned");
}

std::uint32_t AnyBox::Dsp::Connection::GetImpulseResponseSeconds() const { return impulseResponseSeconds; }
std::uint32_t AnyBox::Dsp::Connection::GetMaximumStreamCount() const { return maximumStreamCount; }
std::size_t AnyBox::Dsp::Connection::GetStateCapacity() const { return stateCapacity; }
