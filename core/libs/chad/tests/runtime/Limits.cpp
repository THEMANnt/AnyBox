#include "Tests.hpp"

#include <array>

void AnyBox::Tests::CheckClientLimits() {
    std::vector<float> memory(0x10000 / sizeof(float));
    std::array<XDspConnection*, 8> clients{};
    for (auto& client : clients) client = Connect(memory, 0, 1);
    ExpectException<std::overflow_error>([&] { Connect(memory, 0, 1); });
    for (auto* client : clients) XDspDisconnect(client);

    auto* first = Connect(memory, 100, 1);
    ExpectException<std::length_error>([&] { Connect(memory, 29, 1); });
    auto* second = Connect(memory, 28, 1);
    XDspDisconnect(first);
    XDspDisconnect(second);

    auto* connection = Connect(memory, 0, 1);
    XDspActivationParameters parameters{2, 0, 512, 1, 0, {}};
    XDspStatus* status = nullptr;
    XDspStream* stream = nullptr;
    XDspActivate(connection, &parameters, &status, &stream);
    XDspStatus* sentinelStatus = status;
    XDspStream* sentinelStream = stream;
    ExpectException<std::overflow_error>([&] { XDspActivate(connection, &parameters, &status, &stream); });
    Require(status == sentinelStatus && stream == sentinelStream, "Failed DSP activation changed its output handles");
    XDspDeactivate(stream);

    parameters.type = 0;
    ExpectException<std::invalid_argument>([&] { XDspActivate(connection, &parameters, &status, &stream); });
    parameters.type = 2;
    parameters.frameCount = 511;
    ExpectException<std::invalid_argument>([&] { XDspActivate(connection, &parameters, &status, &stream); });
    ExpectException<std::invalid_argument>([&] { XDspActivate(connection, nullptr, &status, &stream); });
    ExpectException<std::invalid_argument>([&] { XDspSubmitCommand(nullptr, nullptr, nullptr); });
    XDspDisconnect(connection);
    ExpectException<std::invalid_argument>([&] { XDspDisconnect(connection); });
}
