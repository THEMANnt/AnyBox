#ifndef ANYBOX_DSP_STREAM_HPP
#define ANYBOX_DSP_STREAM_HPP

#include "Connection.hpp"
#include "../dsp/Convolver.hpp"

#include <mutex>

namespace AnyBox::Dsp {

class Stream {
public:
    Stream(std::shared_ptr<Connection> connection, const XDspActivationParameters& parameters);
    std::shared_ptr<Connection> GetConnection() const;
    XDspStatus* GetStatus();
    std::size_t GetStateSize() const;
    std::uint32_t Submit(const XDspCommand& command);
    void Close();

private:
    std::shared_ptr<Connection> connection;
    XDspActivationParameters parameters;
    XDspStatus status{};
    std::size_t stateSize = 0;
    std::uint32_t sequence = 0;
    std::unique_ptr<FourierTransform> transform;
    std::unique_ptr<Convolver> convolver;
    std::mutex mutex;
    bool closed = false;
};

}

#endif
