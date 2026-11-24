#include "Connection.hpp"

#include <anybox/Unimplemented.hpp>

#include <cmath>
#include <cstring>
#include <stdexcept>

void AnyBox::XApu::Connection::decode(Stream& stream, const XApuDecodeCommand& command, XApuResult& result) {
    if (!std::isfinite(command.sampleRate) || !std::isfinite(command.ramp)) throw std::invalid_argument("XApu processing parameter is not finite");
    if (command.sampleRate <= 0.0F) throw std::invalid_argument("XApu sample rate must be positive");
    if (command.ramp != 0.0F || command.sampleRate > 48000.0F) ANYBOX_NOT_IMPLEMENTED();
    auto rate = static_cast<std::uint32_t>(command.sampleRate);
    if (static_cast<float>(rate) != command.sampleRate) ANYBOX_NOT_IMPLEMENTED();
    if (rate != 8000 && rate != 12000 && rate != 16000 && rate != 24000 && rate != 48000) ANYBOX_NOT_IMPLEMENTED();
    if (stream.decoded && rate != stream.decoder->GetSampleRate()) ANYBOX_NOT_IMPLEMENTED();
    stream.decoder->SetSampleRate(rate);

    auto* input = command.input.Get();
    auto* output = command.output.Get();
    if (!contains(input, command.inputSize) || !contains(output, command.outputSize)) throw std::invalid_argument("XApu command buffer is outside connection storage");
    if (reinterpret_cast<std::uintptr_t>(output) % alignof(float) != 0) throw std::invalid_argument("XApu PCM output is misaligned");

    auto packet = std::span(static_cast<const std::byte*>(input), command.inputSize);
    auto frames = stream.decoder->GetFrameCount(packet);
    if (command.firstFrame > frames) throw std::out_of_range("XApu first output frame exceeds the packet duration");
    auto outputFrames = frames - command.firstFrame;
    if (command.frameCount != 0 && command.frameCount < outputFrames) throw std::length_error("XApu frame limit is too small for the packet");
    auto channels = stream.decoder->GetChannelCount();
    auto outputSize = outputFrames * channels * static_cast<std::uint32_t>(sizeof(float));
    if (outputSize > command.outputSize) throw std::length_error("XApu PCM output buffer is too small");

    auto decoded = stream.decoder->Decode(packet, stream.samples);
    if (decoded != frames) throw std::runtime_error("XApu decoded packet duration changed");
    stream.decoded = true;

    std::memcpy(output, stream.samples.data() + command.firstFrame * channels, outputSize);

    result.state = 3;
    result.frameCount = outputFrames;
    result.outputSize = outputSize;
    result.output.Set(output);
    result.sampleRate = command.sampleRate;
    result.inputSize = command.inputSize;
    result.input.Set(input);
}
