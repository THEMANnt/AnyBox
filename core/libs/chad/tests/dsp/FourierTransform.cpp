#include "Tests.hpp"
#include "FourierTransform.hpp"

#include <complex>
#include <numbers>
#include <vector>

void AnyBox::Tests::CheckFourierTransform() {
    for (std::size_t size : {16U, 32U}) {
        Dsp::FourierTransform transform(size);
        std::vector<std::complex<float>> values(size);
        for (std::size_t i = 0; i < size; ++i) values[i] = {static_cast<float>(i % 5) - 2.0F, static_cast<float>(i % 3) - 1.0F};
        auto original = values;
        transform.Execute(values, false);

        for (std::size_t bin = 0; bin < size; ++bin) {
            std::complex<double> expected{};
            for (std::size_t frame = 0; frame < size; ++frame) {
                auto angle = -2.0 * std::numbers::pi * static_cast<double>(bin * frame) / static_cast<double>(size);
                expected += std::complex<double>(original[frame]) * std::polar(1.0, angle);
            }
            Near(values[bin].real(), static_cast<float>(expected.real()), "FFT real component differs from direct DFT");
            Near(values[bin].imag(), static_cast<float>(expected.imag()), "FFT imaginary component differs from direct DFT");
        }
        transform.Execute(values, true);
        for (std::size_t i = 0; i < size; ++i) {
            Near(values[i].real(), original[i].real(), "Inverse FFT real component changed");
            Near(values[i].imag(), original[i].imag(), "Inverse FFT imaginary component changed");
        }
    }

    for (std::size_t size : {512U, 1024U, 2048U}) {
        Dsp::FourierTransform transform(size);
        std::vector<float> values(size);
        for (std::size_t i = 0; i < size; ++i) values[i] = 1.0F + (i % 2 ? -0.25F : 0.25F);
        auto original = values;
        transform.ForwardReal(values, values);
        Near(values[0], static_cast<float>(size), "Real FFT DC component is incorrect");
        Near(values[1], static_cast<float>(size) * 0.25F, "Real FFT Nyquist component is incorrect");
        for (std::size_t i = 2; i < size; ++i) Near(values[i], 0.0F, "Real FFT introduced an unexpected frequency");
        transform.InverseReal(values, values);
        for (std::size_t i = 0; i < size; ++i) Near(values[i], original[i], "In-place real FFT round trip failed");
    }
}
