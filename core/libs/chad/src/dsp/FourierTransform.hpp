#ifndef ANYBOX_DSP_FOURIER_TRANSFORM_HPP
#define ANYBOX_DSP_FOURIER_TRANSFORM_HPP

#include <complex>
#include <cstddef>
#include <span>
#include <vector>

namespace AnyBox::Dsp {

class FourierTransform {
public:
    explicit FourierTransform(std::size_t size);
    void Execute(std::span<std::complex<float>> samples, bool inverse) const;
    void ForwardReal(std::span<const float> input, std::span<float> output);
    void InverseReal(std::span<const float> input, std::span<float> output);

private:
    std::vector<std::size_t> permutation;
    std::vector<std::complex<float>> roots;
    std::vector<std::complex<float>> work;
};

}

#endif
