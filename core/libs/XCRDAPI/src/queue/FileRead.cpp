#include "Queue.hpp"
#include "../metrics/Requests.hpp"

#include <limits>
#include <stdexcept>
#include <system_error>

void AnyBox::Storage::Queue::readFile(ReadOperation& operation, std::span<std::byte> destination, std::uint64_t offset) {
    const auto& request = operation.request;
    auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (request.sourceOffset > maximum || offset > maximum - request.sourceOffset || destination.size() > maximum - request.sourceOffset - offset) {
        throw std::overflow_error("DirectStorage file range overflows");
    }
    auto position = request.sourceOffset + offset;
    OVERLAPPED overlapped{};
    overlapped.Offset = static_cast<DWORD>(position);
    overlapped.OffsetHigh = static_cast<DWORD>(position >> 32);
    overlapped.hEvent = readEvent.Get();
    if (!ResetEvent(readEvent.Get())) Windows::ThrowLastError("ResetEvent DirectStorage read");

    WorkStatistics statistics(WorkType::Storage);
    DWORD transferred = 0;
    auto result = ReadFile(operation.file.Get(), destination.data(), static_cast<DWORD>(destination.size()), &transferred, &overlapped);
    if (!result && GetLastError() != ERROR_IO_PENDING) Windows::ThrowLastError("ReadFile DirectStorage");
    if (!GetOverlappedResult(operation.file.Get(), &overlapped, &transferred, TRUE)) Windows::ThrowLastError("GetOverlappedResult DirectStorage");
    if (transferred != destination.size()) throw std::system_error(ERROR_HANDLE_EOF, std::system_category(), "Short DirectStorage read");
}
