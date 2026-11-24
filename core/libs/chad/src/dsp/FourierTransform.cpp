#include "FourierTransform.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>
#include <stdexcept>

AnyBox::Dsp::FourierTransform::FourierTransform(std::size_t size) {
    if (size < 2 || !std::has_single_bit(size)) throw std::invalid_argument("FFT size must be a power of two");
    permutation.resize(size);
    roots.resize(size / 2);
    work.resize(size);

    for (std::size_t i = 1, reversed = 0; i < size; ++i) {
        auto bit = size / 2;
        while (reversed & bit) {
            reversed ^= bit;
            bit >>= 1;
        }
        reversed ^= bit;
        permutation[i] = reversed;
    }
    for (std::size_t i = 0; i < roots.size(); ++i) {
        auto angle = -2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(size);
        roots[i] = {static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))};
    }
}

void AnyBox::Dsp::FourierTransform::Execute(std::span<std::complex<float>> samples, bool inverse) const {
    if (samples.size() != permutation.size()) throw std::invalid_argument("FFT input size does not match its plan");
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (i < permutation[i]) std::swap(samples[i], samples[permutation[i]]);
    }

    for (std::size_t half = 1; half < samples.size(); half *= 2) {
        auto stride = samples.size() / (half * 2);
        for (std::size_t begin = 0; begin < samples.size(); begin += half * 2) {
            for (std::size_t i = 0; i < half; ++i) {
                auto root = roots[i * stride];
                if (inverse) root = std::conj(root);
                auto even = samples[begin + i];
                auto odd = samples[begin + i + half] * root;
                samples[begin + i] = even + odd;
                samples[begin + i + half] = even - odd;
            }
        }
    }
    if (inverse) {
        auto scale = 1.0F / static_cast<float>(samples.size());
        for (auto& sample : samples) sample *= scale;
    }
}
