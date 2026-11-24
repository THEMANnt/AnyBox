#ifndef ANYBOX_DSP_CONNECTION_HPP
#define ANYBOX_DSP_CONNECTION_HPP

#include <chadTypes.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace AnyBox::Dsp {

class Stream;

class Connection {
public:
    Connection(void* memory, std::uint32_t size, std::uint32_t impulseResponseSeconds, std::uint32_t maximumStreamCount);
    void ValidateBuffer(const void* address, std::size_t size) const;
    std::uint32_t GetImpulseResponseSeconds() const;
    std::uint32_t GetMaximumStreamCount() const;
    std::size_t GetStateCapacity() const;

private:
    std::uintptr_t base;
    std::size_t size;
    std::uint32_t impulseResponseSeconds;
    std::uint32_t maximumStreamCount;
    std::size_t stateCapacity;
};

XDspConnection* RegisterConnection(std::shared_ptr<Connection> connection);
std::shared_ptr<Connection> GetConnection(XDspConnection* handle);
void RemoveConnection(XDspConnection* handle);
XDspStream* RegisterStream(std::shared_ptr<Stream> stream);
std::shared_ptr<Stream> GetStream(XDspStream* handle);
void RemoveStream(XDspStream* handle);

}

#endif
