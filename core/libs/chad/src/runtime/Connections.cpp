#include "Registry.hpp"

#include <stdexcept>

AnyBox::Dsp::Registry& AnyBox::Dsp::GetRegistry() {
    static Registry registry;
    return registry;
}

XDspConnection* AnyBox::Dsp::RegisterConnection(std::shared_ptr<Connection> connection) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    if (registry.connections.size() == 8) throw std::overflow_error("DSP client limit has been reached");
    auto seconds = connection->GetImpulseResponseSeconds();
    for (const auto& [handle, entry] : registry.connections) {
        static_cast<void>(handle);
        seconds += entry.connection->GetImpulseResponseSeconds();
    }
    if (seconds > 128) throw std::length_error("DSP aggregate impulse response budget exceeds its limit");

    auto* handle = reinterpret_cast<XDspConnection*>(connection.get());
    if (!registry.connections.emplace(handle, ConnectionEntry{std::move(connection)}).second) throw std::logic_error("DSP client is already registered");
    return handle;
}

std::shared_ptr<AnyBox::Dsp::Connection> AnyBox::Dsp::GetConnection(XDspConnection* handle) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto found = registry.connections.find(handle);
    if (found == registry.connections.end()) throw std::invalid_argument("Invalid DSP client handle");
    return found->second.connection;
}

void AnyBox::Dsp::RemoveConnection(XDspConnection* handle) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto found = registry.connections.find(handle);
    if (found == registry.connections.end()) throw std::invalid_argument("Invalid DSP client handle");
    if (found->second.activeStreams != 0) throw std::logic_error("DSP client has active streams");
    registry.connections.erase(found);
}
