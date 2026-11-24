#include "Connection.hpp"

#include <anybox/Unimplemented.hpp>

#include <chrono>
#include <cstring>
#include <stdexcept>

std::uint32_t AnyBox::XApu::Connection::Enqueue(const XApuCommand* command) {
    if (!command) throw std::invalid_argument("XApu command is null");
    std::lock_guard lock(mutex);
    requireOpen();

    if (command->stream >= streams.size()) throw std::out_of_range("XApu stream index is out of range");
    if (command->options != 0 && (command->kind != 5 || command->options != 4)) ANYBOX_NOT_IMPLEMENTED();
    auto& stream = streams[command->stream];
    if (stream.releasing) throw std::logic_error("XApu stream is being released");
    if (stream.pending == parameters.commandsPerStream) throw std::overflow_error("XApu stream command queue is full");

    XApuResult result{};
    result.command = *command;
    auto start = std::chrono::steady_clock::now();
    switch (command->kind) {
        case 1: {
            XApuActivateCommand activation;
            std::memcpy(static_cast<void*>(&activation), command, sizeof(activation));
            activate(stream, activation);
            break;
        }
        case 2:
            if (!stream.decoder) throw std::logic_error("XApu stream is not active");
            stream.releasing = true;
            break;
        case 3: {
            if (!stream.decoder) throw std::logic_error("XApu stream is not active");
            XApuDecodeCommand process;
            std::memcpy(static_cast<void*>(&process), command, sizeof(process));
            decode(stream, process, result);
            break;
        }
        case 5:
            if (!stream.decoder) throw std::logic_error("XApu stream is not active");
            stream.decoder->Reset();
            stream.decoded = false;
            break;
        default:
            ANYBOX_NOT_IMPLEMENTED();
    }

    auto duration = std::chrono::steady_clock::now() - start;
    result.processingTime.Set(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()));
    results[(head + count) % results.size()] = result;
    ++count;
    ++stream.pending;
    return ++submitted;
}

void AnyBox::XApu::Connection::activate(Stream& stream, const XApuActivateCommand& command) {
    if (stream.decoder) throw std::logic_error("XApu stream is already active");
    for (auto parameter : command.parameters) if (parameter != 0) ANYBOX_NOT_IMPLEMENTED();

    auto decoder = std::make_unique<OpusDecoder>(command.channels);
    std::vector<float> samples(static_cast<std::size_t>(command.channels) * 5760);
    stream.decoded = false;
    stream.decoder = std::move(decoder);
    stream.samples = std::move(samples);
}
