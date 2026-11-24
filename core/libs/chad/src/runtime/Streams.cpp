#include "Registry.hpp"

#include <stdexcept>

XDspStream* AnyBox::Dsp::RegisterStream(std::shared_ptr<Stream> stream) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto owner = stream->GetConnection();
    auto* client = reinterpret_cast<XDspConnection*>(owner.get());
    auto found = registry.connections.find(client);
    if (found == registry.connections.end()) throw std::logic_error("DSP client has been disconnected");
    auto& entry = found->second;
    if (entry.activeStreams == owner->GetMaximumStreamCount()) throw std::overflow_error("DSP stream limit has been reached");
    if (stream->GetStateSize() > owner->GetStateCapacity() - entry.stateUsed) throw std::bad_alloc();

    auto* handle = reinterpret_cast<XDspStream*>(stream.get());
    auto size = stream->GetStateSize();
    if (!registry.streams.emplace(handle, std::move(stream)).second) throw std::logic_error("DSP stream is already registered");
    ++entry.activeStreams;
    entry.stateUsed += size;
    return handle;
}

std::shared_ptr<AnyBox::Dsp::Stream> AnyBox::Dsp::GetStream(XDspStream* handle) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto found = registry.streams.find(handle);
    if (found == registry.streams.end()) throw std::invalid_argument("Invalid DSP stream handle");
    return found->second;
}

void AnyBox::Dsp::RemoveStream(XDspStream* handle) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto found = registry.streams.find(handle);
    if (found == registry.streams.end()) throw std::invalid_argument("Invalid DSP stream handle");
    auto stream = found->second;
    stream->Close();
    auto* owner = reinterpret_cast<XDspConnection*>(stream->GetConnection().get());
    auto& entry = registry.connections.at(owner);
    --entry.activeStreams;
    entry.stateUsed -= stream->GetStateSize();
    registry.streams.erase(found);
}
