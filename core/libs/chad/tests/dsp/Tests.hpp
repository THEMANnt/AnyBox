#ifndef ANYBOX_DSP_TESTS_HPP
#define ANYBOX_DSP_TESTS_HPP

#include <cmath>
#include <stdexcept>

namespace AnyBox::Tests {

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

inline void Near(float actual, float expected, const char* message) {
    Require(std::isfinite(actual) && std::abs(actual - expected) <= 0.0005F * (1.0F + std::abs(expected)), message);
}

void CheckFourierTransform();
void CheckConvolution();

}

#endif
