#include <xapu.hpp>

#include <stdexcept>

namespace {

template<class TException, class TCallable>
void ExpectException(TCallable callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected XApu validation error was not raised");
}

}

void CheckConnectionErrors() {
    XApuConnectionParameters parameters{1, 3, 2, 4096, 0, 0};
    XApuBuffer buffer{};
    XApuConnection* connection = nullptr;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, nullptr); });
    ExpectException<std::invalid_argument>([&] { XApuConnect(nullptr, &buffer, &connection); });
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, nullptr, &connection); });

    parameters.processor = 0;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.processor = 2;
    ExpectException<std::runtime_error>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.processor = 1;
    parameters.streamCount = 0;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.streamCount = 501;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.streamCount = 3;
    parameters.commandsPerStream = 17;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.commandsPerStream = 2;
    parameters.flags = 16;
    ExpectException<std::invalid_argument>([&] { XApuConnect(&parameters, &buffer, &connection); });
    parameters.flags = 0;
    XApuConnect(&parameters, &buffer, &connection);

    ExpectException<std::invalid_argument>([] { XApuGetResultCount(nullptr, nullptr); });
    ExpectException<std::invalid_argument>([&] { XApuEnqueueCommand(connection, nullptr, nullptr); });
    ExpectException<std::invalid_argument>([&] { XApuDequeueResult(connection, nullptr); });
    XApuCommand command{};
    command.kind = 5;
    command.stream = 3;
    ExpectException<std::out_of_range>([&] { XApuEnqueueCommand(connection, &command, nullptr); });
    command.stream = 0;
    ExpectException<std::logic_error>([&] { XApuEnqueueCommand(connection, &command, nullptr); });

    XApuActivateCommand activation{};
    activation.command.kind = 1;
    activation.channels = 3;
    ExpectException<std::invalid_argument>([&] { XApuEnqueueCommand(connection, &activation.command, nullptr); });
    activation.channels = 1;
    XApuEnqueueCommand(connection, &activation.command, nullptr);
    XApuResult result{};
    XApuDequeueResult(connection, &result);
    ExpectException<std::logic_error>([&] { XApuEnqueueCommand(connection, &activation.command, nullptr); });

    XApuDecodeCommand decode{};
    decode.command.kind = 3;
    decode.sampleRate = 0.0F;
    ExpectException<std::invalid_argument>([&] { XApuEnqueueCommand(connection, &decode.command, nullptr); });
    decode.sampleRate = 1.0F;
    ExpectException<std::runtime_error>([&] { XApuEnqueueCommand(connection, &decode.command, nullptr); });
    decode.sampleRate = 48000.0F;
    decode.inputSize = 1;
    decode.input.Set(&command);
    decode.outputSize = 4;
    decode.output.Set(buffer.data);
    ExpectException<std::invalid_argument>([&] { XApuEnqueueCommand(connection, &decode.command, nullptr); });
    if (XApuGetResultCount(connection, nullptr) != 0) throw std::runtime_error("Failed XApu command queued a fabricated result");

    command.kind = 99;
    ExpectException<std::runtime_error>([&] { XApuEnqueueCommand(connection, &command, nullptr); });
    XApuDisconnect(connection);
    ExpectException<std::invalid_argument>([&] { XApuDisconnect(connection); });
}
