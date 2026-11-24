#include "FourierTransform.hpp"

#include <stdexcept>

void AnyBox::Dsp::FourierTransform::ForwardReal(std::span<const float> input, std::span<float> output) {
    if (input.size() != work.size() || output.size() != work.size()) throw std::invalid_argument("Real FFT buffer size does not match its plan");
    for (std::size_t i = 0; i < work.size(); ++i) work[i] = {input[i], 0.0F};
    Execute(work, false);

    output[0] = work.front().real();
    output[1] = work[work.size() / 2].real();
    for (std::size_t i = 1; i < work.size() / 2; ++i) {
        output[i * 2] = work[i].real();
        output[i * 2 + 1] = work[i].imag();
    }
}

void AnyBox::Dsp::FourierTransform::InverseReal(std::span<const float> input, std::span<float> output) {
    if (input.size() != work.size() || output.size() != work.size()) throw std::invalid_argument("Real inverse FFT buffer size does not match its plan");
    work[0] = {input[0], 0.0F};
    work[work.size() / 2] = {input[1], 0.0F};
    for (std::size_t i = 1; i < work.size() / 2; ++i) {
        work[i] = {input[i * 2], input[i * 2 + 1]};
        work[work.size() - i] = std::conj(work[i]);
    }

    Execute(work, true);
    for (std::size_t i = 0; i < work.size(); ++i) output[i] = work[i].real();
}
