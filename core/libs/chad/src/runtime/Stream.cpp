#include "Stream.hpp"

#include <anybox/Unimplemented.hpp>

#include <stdexcept>

AnyBox::Dsp::Stream::Stream(std::shared_ptr<Connection> connection, const XDspActivationParameters& parameters) : connection(std::move(connection)), parameters(parameters) {
    if (parameters.type < 1 || parameters.type > 3) throw std::invalid_argument("Invalid DSP processing type");
    if (parameters.channels < 1 || parameters.channels > 2) throw std::invalid_argument("Invalid DSP channel count");
    if (parameters.frameCount != 512 && parameters.frameCount != 1024 && parameters.frameCount != 2048) throw std::invalid_argument("Invalid DSP frame count");

    if (parameters.type != 1) {
        if (parameters.channels != 1) throw std::invalid_argument("DSP FFT requires one channel");
        if (parameters.flags != 0 || parameters.impulseResponseFloatCount != 0 || parameters.impulseResponse.Get() != nullptr) ANYBOX_NOT_IMPLEMENTED();
        transform = std::make_unique<FourierTransform>(parameters.frameCount);
        return;
    }
    if (parameters.frameCount == 2048) throw std::invalid_argument("Invalid convolution frame count");
    if ((parameters.flags & ~10U) != 0) ANYBOX_NOT_IMPLEMENTED();
    if ((parameters.flags & 10) != 0 && parameters.channels != 2) throw std::invalid_argument("Stereo convolution requires two output channels");

    auto partitionSize = parameters.frameCount * 2;
    if (parameters.impulseResponseFloatCount % partitionSize != 0) throw std::invalid_argument("DSP impulse response does not contain complete partitions");
    auto partitions = parameters.impulseResponseFloatCount / partitionSize;
    auto roundedPartitions = (partitions + 7) & ~7U;
    if (partitions < 9 || roundedPartitions > 2048) throw std::invalid_argument("DSP impulse response exceeds its partition limits");
    if (!parameters.impulseResponse.Get()) throw std::invalid_argument("DSP impulse response is null");

    auto filterChannels = (parameters.flags & 8) != 0 ? 2U : 1U;
    auto stateBytes = 8ULL * parameters.frameCount * (filterChannels * partitions + parameters.channels * (roundedPartitions + 8));
    stateSize = static_cast<std::size_t>(filterChannels + parameters.channels + (stateBytes + 8191) / 8192) * 8192;
    auto count = static_cast<std::size_t>(parameters.impulseResponseFloatCount) * filterChannels;
    convolver = std::make_unique<Convolver>(parameters.frameCount, parameters.channels, (parameters.flags & 2) != 0, filterChannels == 2, std::span(parameters.impulseResponse.Get(), count));
}

std::shared_ptr<AnyBox::Dsp::Connection> AnyBox::Dsp::Stream::GetConnection() const { return connection; }
XDspStatus* AnyBox::Dsp::Stream::GetStatus() { return &status; }
std::size_t AnyBox::Dsp::Stream::GetStateSize() const { return stateSize; }

void AnyBox::Dsp::Stream::Close() {
    std::lock_guard lock(mutex);
    if (closed) throw std::logic_error("DSP stream is already closed");
    closed = true;
}
