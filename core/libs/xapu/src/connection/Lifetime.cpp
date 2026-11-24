#include "Connection.hpp"

#include <stdexcept>

AnyBox::XApu::Connection::Connection(const XApuConnectionParameters& parameters) : parameters(parameters), data(std::make_unique<std::byte[]>(parameters.bufferSize ? parameters.bufferSize : 1)), streams(parameters.streamCount), results(parameters.streamCount * parameters.commandsPerStream) {}

void* AnyBox::XApu::Connection::GetBuffer() const {
    return data.get();
}

void AnyBox::XApu::Connection::requireOpen() const {
    if (closed) throw std::logic_error("XApu connection is closed");
}

bool AnyBox::XApu::Connection::contains(const void* address, std::size_t size) const {
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto base = reinterpret_cast<std::uintptr_t>(data.get());
    return value >= base && value - base <= parameters.bufferSize && size <= parameters.bufferSize - (value - base);
}

void AnyBox::XApu::Connection::Close() {
    std::lock_guard lock(mutex);
    requireOpen();
    if (count != 0) throw std::logic_error("XApu connection has outstanding results");
    closed = true;
}

std::uint32_t AnyBox::XApu::Connection::Count(std::uint32_t* dequeuedCount) {
    std::lock_guard lock(mutex);
    requireOpen();
    if (dequeuedCount) *dequeuedCount = dequeued;
    return static_cast<std::uint32_t>(count);
}

void AnyBox::XApu::Connection::Dequeue(XApuResult* result) {
    if (!result) throw std::invalid_argument("XApu result output is null");

    std::lock_guard lock(mutex);
    requireOpen();
    if (count == 0) throw std::underflow_error("XApu result queue is empty");

    const auto& current = results[head];
    auto& stream = streams[current.command.stream];
    if (current.command.kind == 2) {
        stream.decoder.reset();
        stream.samples.clear();
        stream.releasing = false;
    }

    *result = current;
    --stream.pending;
    --count;
    ++dequeued;
    head = (head + 1) % results.size();
}
