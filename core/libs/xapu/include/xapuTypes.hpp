#ifndef ANYBOX_XAPU_TYPES_HPP
#define ANYBOX_XAPU_TYPES_HPP

#include <anybox/AbiValue.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

struct XApuConnectionParameters {
    std::uint32_t processor;
    std::uint32_t streamCount;
    std::uint32_t commandsPerStream;
    std::uint32_t bufferSize;
    std::uint32_t frameCount;
    std::uint32_t flags;
};

static_assert(sizeof(XApuConnectionParameters) == 24);

struct XApuBuffer {
    void* data;
    void* readyEvent;
};
struct XApuConnection;
struct XApuCommand {
    std::uint32_t kind;
    std::uint32_t stream;
    std::uint32_t sequence;
    std::uint32_t options;
    AnyBox::AbiValue<void*> context;
};

struct XApuActivateCommand {
    XApuCommand command;
    std::uint32_t channels;
    std::array<std::uint32_t, 7> parameters;
};

struct XApuDecodeCommand {
    XApuCommand command;
    float ramp;
    float sampleRate;
    std::uint32_t firstFrame;
    std::uint32_t frameCount;
    std::uint32_t inputSize;
    AnyBox::AbiValue<const void*> input;
    std::uint32_t outputSize;
    AnyBox::AbiValue<void*> output;
};

struct XApuResult {
    XApuCommand command;
    std::uint32_t state;
    AnyBox::AbiValue<std::uint64_t> stateData;
    AnyBox::AbiValue<std::uint64_t> processingTime;
    std::int32_t error;
    std::uint32_t frameCount;
    std::uint32_t outputSize;
    AnyBox::AbiValue<void*> output;
    float sampleRate;
    std::uint32_t inputSize;
    AnyBox::AbiValue<const void*> input;
};

static_assert(sizeof(XApuBuffer) == 16);
static_assert(sizeof(XApuCommand) == 24);
static_assert(std::is_trivially_copyable_v<XApuActivateCommand>);
static_assert(std::is_trivially_copyable_v<XApuDecodeCommand>);
static_assert(sizeof(XApuActivateCommand) == 56);
static_assert(sizeof(XApuDecodeCommand) == 64);
static_assert(offsetof(XApuDecodeCommand, sampleRate) == 28);
static_assert(offsetof(XApuDecodeCommand, input) == 44);
static_assert(sizeof(XApuResult) == 80);
static_assert(offsetof(XApuResult, error) == 44);
static_assert(offsetof(XApuResult, outputSize) == 52);
static_assert(offsetof(XApuResult, sampleRate) == 64);

#endif
