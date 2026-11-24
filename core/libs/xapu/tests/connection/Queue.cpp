#include <xapu.hpp>
#include "../../src/codec/OpusDecoder.hpp"

#include <opus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void ExpectException(TCallable callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected XApu error was not raised");
}

void CheckQueue(std::uint32_t channels, std::uint32_t sampleRate) {
    auto frames = sampleRate / 50;
    XApuConnectionParameters parameters{1, 7, 2, 0x20000, 0, 1};
    XApuBuffer buffer{};
    XApuConnection* connection = nullptr;
    Require(XApuConnect(&parameters, &buffer, &connection) == 0 && connection && buffer.data, "XApu connection failed");
    Require(buffer.readyEvent == nullptr, "XApu returned an unsolicited event");

    XApuActivateCommand activation{};
    activation.command.kind = 1;
    activation.command.stream = 5;
    activation.command.sequence = 37;
    activation.command.context.Set(&parameters);
    activation.channels = channels;
    std::uint32_t submitted = 0;
    XApuEnqueueCommand(connection, &activation.command, &submitted);
    Require(submitted == 1, "XApu submitted counter changed");

    int status = 0;
    std::unique_ptr<OpusEncoder, decltype(&opus_encoder_destroy)> encoder(opus_encoder_create(48000, static_cast<int>(channels), OPUS_APPLICATION_AUDIO, &status), opus_encoder_destroy);
    Require(status == OPUS_OK && encoder, "Queue test encoder creation failed");
    std::vector<float> source(960 * channels);
    for (std::size_t i = 0; i < source.size(); ++i) source[i] = static_cast<float>(0.25 * std::sin(static_cast<double>(i) * 0.08));
    auto* input = static_cast<unsigned char*>(buffer.data);
    int bytes = 0;
    for (int i = 0; i < 8; ++i) bytes = opus_encode_float(encoder.get(), source.data(), 960, input, 4000);
    Require(bytes > 0, "Queue test packet encoding failed");

    auto* output = reinterpret_cast<float*>(input + 4096);
    output[-1] = -99.0F;
    std::fill_n(output, 960 * channels + 1, -99.0F);
    XApuDecodeCommand decode{};
    decode.command.kind = 3;
    decode.command.stream = 5;
    decode.sampleRate = static_cast<float>(sampleRate);
    decode.frameCount = 960;
    decode.inputSize = static_cast<std::uint32_t>(bytes);
    decode.input.Set(input);
    decode.outputSize = 960 * channels * sizeof(float);
    decode.output.Set(output);
    XApuEnqueueCommand(connection, &decode.command, &submitted);
    Require(submitted == 2, "XApu submitted counter does not count all commands");
    ExpectException<std::overflow_error>([&] { XApuEnqueueCommand(connection, &decode.command, nullptr); });
    ExpectException<std::logic_error>([&] { XApuDisconnect(connection); });

    std::uint32_t dequeued = 99;
    Require(XApuGetResultCount(connection, &dequeued) == 2 && dequeued == 0, "XApu result counters are inconsistent");
    XApuResult result{};
    XApuDequeueResult(connection, &result);
    Require(result.command.kind == 1 && result.command.sequence == 37 && result.command.context.Get() == &parameters, "XApu activation result lost command identity");
    Require(XApuGetResultCount(connection, &dequeued) == 1 && dequeued == 1, "XApu dequeue counter changed");
    XApuDequeueResult(connection, &result);
    Require(result.command.kind == 3 && result.error == 0 && result.frameCount == frames, "XApu decoding result is invalid");
    Require(result.input.Get() == input && result.inputSize == static_cast<std::uint32_t>(bytes), "XApu result input metadata changed");
    Require(result.output.Get() == output && result.outputSize == frames * channels * sizeof(float), "XApu result output metadata changed");
    Require(result.sampleRate == static_cast<float>(sampleRate), "XApu result sample rate changed from the title command");
    Require(output[-1] == -99.0F && output[frames * channels] == -99.0F, "XApu PCM write overran its output");

    AnyBox::XApu::OpusDecoder reference(channels, sampleRate);
    std::vector<float> interleaved(frames * channels);
    reference.Decode(std::as_bytes(std::span(input, static_cast<std::size_t>(bytes))), interleaved);
    Require(std::equal(interleaved.begin(), interleaved.end(), output), "XApu PCM does not use the interleaved order consumed by the title");

    auto alternateRate = sampleRate == 48000 ? 24000U : 48000U;
    decode.sampleRate = static_cast<float>(alternateRate);
    ExpectException<std::runtime_error>([&] { XApuEnqueueCommand(connection, &decode.command, nullptr); });
    Require(XApuGetResultCount(connection, nullptr) == 0, "Unsupported XApu rate change queued a fabricated result");

    XApuCommand reset{};
    reset.kind = 5;
    reset.stream = 5;
    reset.options = 4;
    XApuEnqueueCommand(connection, &reset, &submitted);
    XApuDequeueResult(connection, &result);
    XApuEnqueueCommand(connection, &decode.command, &submitted);
    XApuDequeueResult(connection, &result);
    Require(result.sampleRate == static_cast<float>(alternateRate) && result.frameCount == alternateRate / 50, "XApu reset did not allow a new native sample rate");
    Require(XApuGetResultCount(connection, &dequeued) == 0 && dequeued == 4 && submitted == 4, "XApu cumulative counters reset with the stream");
    reference.SetSampleRate(alternateRate);
    interleaved.resize(alternateRate / 50 * channels);
    auto packet = std::as_bytes(std::span(input, static_cast<std::size_t>(bytes)));
    reference.Decode(packet, interleaved);
    reference.Decode(packet, interleaved);
    decode.firstFrame = 17;
    auto trimmed = alternateRate / 50 - decode.firstFrame;
    output[trimmed * channels] = -99.0F;
    XApuEnqueueCommand(connection, &decode.command, &submitted);
    XApuDequeueResult(connection, &result);
    Require(result.frameCount == trimmed && result.outputSize == trimmed * channels * sizeof(float), "XApu skip metadata changed");
    Require(std::equal(interleaved.begin() + decode.firstFrame * channels, interleaved.end(), output), "XApu skip did not preserve interleaved frames and decoder history");
    Require(output[-1] == -99.0F && output[trimmed * channels] == -99.0F, "XApu skip overran its output");
    Require(XApuGetResultCount(connection, &dequeued) == 0 && submitted == 5 && dequeued == 5, "XApu skip changed cumulative counters");
    ExpectException<std::underflow_error>([&] { XApuDequeueResult(connection, &result); });

    XApuCommand release{};
    release.kind = 2;
    release.stream = 5;
    XApuEnqueueCommand(connection, &release, nullptr);
    ExpectException<std::logic_error>([&] { XApuEnqueueCommand(connection, &activation.command, nullptr); });
    XApuDequeueResult(connection, &result);
    XApuEnqueueCommand(connection, &activation.command, nullptr);
    XApuDequeueResult(connection, &result);
    Require(XApuDisconnect(connection) == 0, "XApu disconnection failed");
    ExpectException<std::invalid_argument>([&] { XApuGetResultCount(connection, nullptr); });
}

}

void CheckQueues() {
    for (auto sampleRate : {8000U, 12000U, 16000U, 24000U, 48000U}) {
        CheckQueue(1, sampleRate);
        CheckQueue(2, sampleRate);
    }
}
