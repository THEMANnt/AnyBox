#include "WorkState.hpp"
#include "XmaContext.hpp"
#include "../memory/Heap.hpp"
#include <anybox/Unimplemented.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

std::optional<AnyBox::AcpHal::XmaWorkState> AnyBox::AcpHal::ReadXmaWorkState(std::uint32_t address) {
    auto memory = GetHeap().GetMemory(address, 512);
    XmaWorkState state{};
    std::memcpy(&state, memory.data(), sizeof(state));
    if (state.signature != XmaWorkSignature) {
        if (std::ranges::all_of(memory, [](auto value) { return value == std::byte{}; })) return std::nullopt;
        ANYBOX_NOT_IMPLEMENTED();
    }
    if (state.generation == 0) throw std::invalid_argument("XMA work state has an invalid generation");
    return state;
}

void AnyBox::AcpHal::XmaContext::publishState() {
    if (sequence == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("XMA work state generation exhausted");
    auto memory = GetHeap().GetMemory(words[8], 512);
    generation = ++sequence;
    XmaWorkState state{XmaWorkSignature, generation, index, GetFormat()};
    std::memcpy(memory.data(), &state, sizeof(state));
}

void AnyBox::AcpHal::XmaContext::validateWork() const {
    if (!active || !decoder) throw std::logic_error("XMA context is not active");
    auto state = ReadXmaWorkState(words[8]);
    if (!state && generation == 0) return;
    if (!state || state->generation != generation || state->context != index || state->format != GetFormat()) ANYBOX_NOT_IMPLEMENTED();
}

void AnyBox::AcpHal::XmaContext::Stop() noexcept { active = false; }
bool AnyBox::AcpHal::XmaContext::IsActive() const noexcept { return active; }
std::uint64_t AnyBox::AcpHal::XmaContext::GetGeneration() const noexcept { return generation; }
std::uint32_t AnyBox::AcpHal::XmaContext::GetFormat() const noexcept { return (rateIndex << 27) | ((channels - 1) << 29); }

AnyBox::Codecs::XmaFrameDecoder AnyBox::AcpHal::XmaContext::TakeDecoder() {
    if (active || !decoder) ANYBOX_NOT_IMPLEMENTED();
    auto result = std::move(*decoder);
    decoder.reset();
    return result;
}
