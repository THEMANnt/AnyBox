#include "Connection.hpp"

#include <stdexcept>
#include <unordered_map>

namespace {

std::mutex registryMutex;
std::unordered_map<XApuConnection*, std::shared_ptr<AnyBox::XApu::Connection>> connections;

}

XApuConnection* AnyBox::XApu::RegisterConnection(std::shared_ptr<Connection> connection) {
    auto* handle = reinterpret_cast<XApuConnection*>(~reinterpret_cast<std::uintptr_t>(connection.get()));
    std::lock_guard lock(registryMutex);
    if (!connections.emplace(handle, std::move(connection)).second) throw std::logic_error("XApu connection handle is already registered");
    return handle;
}

std::shared_ptr<AnyBox::XApu::Connection> AnyBox::XApu::GetConnection(XApuConnection* handle) {
    std::lock_guard lock(registryMutex);
    auto found = connections.find(handle);
    if (found == connections.end()) throw std::invalid_argument("Invalid XApu connection handle");
    return found->second;
}

void AnyBox::XApu::RemoveConnection(XApuConnection* handle) {
    auto connection = GetConnection(handle);
    connection->Close();
    std::lock_guard lock(registryMutex);
    if (connections.erase(handle) != 1) throw std::logic_error("XApu connection is not registered");
}
