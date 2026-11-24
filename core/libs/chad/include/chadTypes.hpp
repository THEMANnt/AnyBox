#ifndef ANYBOX_CHAD_TYPES_HPP
#define ANYBOX_CHAD_TYPES_HPP

#include <anybox/AbiValue.hpp>

#include <cstddef>
#include <cstdint>

struct XDspConnection;
struct XDspStream;

struct XDspActivationParameters {
    std::uint32_t type;
    std::uint32_t flags;
    std::uint32_t frameCount;
    std::uint32_t channels;
    std::uint32_t impulseResponseFloatCount;
    AnyBox::AbiValue<const float*> impulseResponse;
};

struct XDspCommand {
    float startGain;
    float endGain;
    const float* input;
    float* output;
    const void* auxiliary;
};

struct XDspStatus {
    std::uint32_t submitted;
    std::uint32_t completed;
    std::uint32_t failed;
    std::int32_t error;
};

static_assert(sizeof(XDspActivationParameters) == 28);
static_assert(offsetof(XDspActivationParameters, impulseResponse) == 20);
static_assert(sizeof(XDspCommand) == 32);
static_assert(sizeof(XDspStatus) == 16);

#endif
