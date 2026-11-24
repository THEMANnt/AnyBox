#ifndef ANYBOX_DSP_CONVOLVER_HPP
#define ANYBOX_DSP_CONVOLVER_HPP

#include "FourierTransform.hpp"

#include <cstdint>

namespace AnyBox::Dsp {

class Convolver {
public:
    Convolver(std::uint32_t frameCount, std::uint32_t outputChannels, bool stereoInput, bool stereoFilter, std::span<const float> coefficients);
    void Process(std::span<const float> input, std::span<float> output);

private:
    std::size_t frameCount;
    std::size_t outputChannels;
    std::size_t inputChannels;
    std::size_t filterChannels;
    std::size_t partitions;
    std::size_t head = 0;
    FourierTransform transform;
    std::vector<std::complex<float>> filters;
    std::vector<std::complex<float>> history;
    std::vector<std::complex<float>> work;
    std::vector<float> overlap;
};

}

#endif
