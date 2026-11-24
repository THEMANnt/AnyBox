#include "Stream.hpp"

#include <anybox/Unimplemented.hpp>

#include <atomic>
#include <cmath>
#include <stdexcept>

std::uint32_t AnyBox::Dsp::Stream::Submit(const XDspCommand& command) {
    std::lock_guard lock(mutex);
    if (closed) throw std::logic_error("DSP stream is closed");
    if (!std::isfinite(command.startGain) || !std::isfinite(command.endGain)) throw std::invalid_argument("DSP gain is not finite");
    if (command.startGain != 1.0F || command.endGain != 1.0F || command.auxiliary != nullptr) ANYBOX_NOT_IMPLEMENTED();

    auto inputChannels = parameters.type == 1 && (parameters.flags & 2) != 0 ? 2U : 1U;
    auto inputSize = static_cast<std::size_t>(parameters.frameCount) * inputChannels;
    auto outputSize = static_cast<std::size_t>(parameters.frameCount) * parameters.channels;
    connection->ValidateBuffer(command.input, inputSize * sizeof(float));
    connection->ValidateBuffer(command.output, outputSize * sizeof(float));
    auto input = std::span(command.input, inputSize);
    auto output = std::span(command.output, outputSize);

    if (convolver) convolver->Process(input, output);
    else if (parameters.type == 2) transform->ForwardReal(input, output);
    else transform->InverseReal(input, output);

    ++sequence;
    std::atomic_ref(status.submitted).store(sequence, std::memory_order_relaxed);
    std::atomic_ref(status.completed).store(sequence, std::memory_order_release);
    return sequence;
}
