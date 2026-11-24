#include "Convolver.hpp"

#include <algorithm>
#include <stdexcept>

AnyBox::Dsp::Convolver::Convolver(std::uint32_t frameCount, std::uint32_t outputChannels, bool stereoInput, bool stereoFilter, std::span<const float> coefficients) : frameCount(frameCount), outputChannels(outputChannels), inputChannels(stereoInput ? 2 : 1), filterChannels(stereoFilter ? 2 : 1), partitions(0), transform(static_cast<std::size_t>(frameCount) * 2) {
    if (outputChannels < 1 || outputChannels > 2 || inputChannels > outputChannels || filterChannels > outputChannels) throw std::invalid_argument("Invalid convolution channel configuration");
    auto partitionSize = static_cast<std::size_t>(frameCount) * 2;
    if (coefficients.empty() || coefficients.size() % (partitionSize * filterChannels) != 0) throw std::invalid_argument("Invalid convolution coefficient count");
    partitions = coefficients.size() / (partitionSize * filterChannels);
    if (partitions > 2048) throw std::length_error("Convolution impulse response exceeds its partition limit");

    filters.resize(filterChannels * partitions * (frameCount + 1));
    history.resize(inputChannels * partitions * (frameCount + 1));
    work.resize(partitionSize);
    overlap.resize(static_cast<std::size_t>(frameCount) * outputChannels);
    for (std::size_t channel = 0; channel < filterChannels; ++channel) {
        for (std::size_t partition = 0; partition < partitions; ++partition) {
            auto source = coefficients.subspan((channel * partitions + partition) * partitionSize, partitionSize);
            auto begin = (channel * partitions + partition) * (frameCount + 1);
            filters[begin] = {source[0], 0.0F};
            filters[begin + frameCount] = {source[1], 0.0F};
            for (std::size_t bin = 1; bin < frameCount; ++bin) filters[begin + bin] = {source[bin * 2], source[bin * 2 + 1]};
        }
    }
}

void AnyBox::Dsp::Convolver::Process(std::span<const float> input, std::span<float> output) {
    if (input.size() != frameCount * inputChannels || output.size() != frameCount * outputChannels) throw std::invalid_argument("Convolution buffer size does not match its stream");
    for (std::size_t channel = 0; channel < inputChannels; ++channel) {
        for (std::size_t frame = 0; frame < frameCount; ++frame) work[frame] = {input[channel * frameCount + frame], 0.0F};
        std::fill(work.begin() + frameCount, work.end(), std::complex<float>{});
        transform.Execute(work, false);
        auto begin = history.begin() + (channel * partitions + head) * (frameCount + 1);
        std::copy_n(work.begin(), frameCount + 1, begin);
    }

    for (std::size_t channel = 0; channel < outputChannels; ++channel) {
        std::fill(work.begin(), work.end(), std::complex<float>{});
        auto inputChannel = inputChannels == 1 ? 0 : channel;
        auto filterChannel = filterChannels == 1 ? 0 : channel;
        for (std::size_t partition = 0; partition < partitions; ++partition) {
            auto index = (head + partitions - partition) % partitions;
            auto historyBegin = (inputChannel * partitions + index) * (frameCount + 1);
            auto filterBegin = (filterChannel * partitions + partition) * (frameCount + 1);
            for (std::size_t bin = 0; bin <= frameCount; ++bin) work[bin] += history[historyBegin + bin] * filters[filterBegin + bin];
        }
        for (std::size_t bin = 1; bin < frameCount; ++bin) work[work.size() - bin] = std::conj(work[bin]);
        transform.Execute(work, true);
        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            auto index = channel * frameCount + frame;
            output[index] = work[frame].real() + overlap[index];
            overlap[index] = work[frameCount + frame].real();
        }
    }
    head = (head + 1) % partitions;
}
