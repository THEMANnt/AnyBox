#ifndef ANYBOX_XAPU_CONNECTION_HPP
#define ANYBOX_XAPU_CONNECTION_HPP

#include "../codec/OpusDecoder.hpp"

#include <xapuTypes.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace AnyBox::XApu {

class Connection {
public:
    explicit Connection(const XApuConnectionParameters& parameters);
    void* GetBuffer() const;
    std::uint32_t Enqueue(const XApuCommand* command);
    void Dequeue(XApuResult* result);
    std::uint32_t Count(std::uint32_t* dequeuedCount);
    void Close();

private:
    struct Stream {
        std::unique_ptr<OpusDecoder> decoder;
        std::vector<float> samples;
        std::uint32_t pending = 0;
        bool releasing = false;
        bool decoded = false;
    };
    void activate(Stream& stream, const XApuActivateCommand& command);
    void decode(Stream& stream, const XApuDecodeCommand& command, XApuResult& result);
    bool contains(const void* address, std::size_t size) const;
    void requireOpen() const;
    XApuConnectionParameters parameters;
    std::unique_ptr<std::byte[]> data;
    std::vector<Stream> streams;
    std::vector<XApuResult> results;
    std::mutex mutex;
    std::size_t head = 0;
    std::size_t count = 0;
    std::uint32_t submitted = 0;
    std::uint32_t dequeued = 0;
    bool closed = false;
};

XApuConnection* RegisterConnection(std::shared_ptr<Connection> connection);
std::shared_ptr<Connection> GetConnection(XApuConnection* handle);
void RemoveConnection(XApuConnection* handle);

}

#endif
