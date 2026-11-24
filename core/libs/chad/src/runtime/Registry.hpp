#ifndef ANYBOX_DSP_REGISTRY_HPP
#define ANYBOX_DSP_REGISTRY_HPP

#include "Stream.hpp"

#include <unordered_map>

namespace AnyBox::Dsp {

struct ConnectionEntry {
    std::shared_ptr<Connection> connection;
    std::size_t stateUsed = 0;
    std::uint32_t activeStreams = 0;
};

struct Registry {
    std::mutex mutex;
    std::unordered_map<XDspConnection*, ConnectionEntry> connections;
    std::unordered_map<XDspStream*, std::shared_ptr<Stream>> streams;
};

Registry& GetRegistry();

}

#endif
