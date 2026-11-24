#include "Queue.hpp"

#include <bit>
#include <system_error>

void AnyBox::Storage::Queue::process() {
    std::int32_t groupResult = 0;
    std::unique_lock lock(mutex);
    for (;;) {
        wake.wait(lock, [this] { return submitted != 0 || closing; });
        if (submitted == 0 && closing) {
            statisticsRegistration.Close();
            closed = true;
            drained.notify_all();
            return;
        }
        auto operation = std::move(operations[head]);
        operations[head].emplace<std::monostate>();
        head = (head + 1) % operations.size();
        --count;
        --submitted;
        spaceAvailable.notify_one();
        lock.unlock();
        std::int32_t failure = 0;
        try {
            if (auto* request = std::get_if<ReadOperation>(&operation)) read(*request);
            else if (auto* status = std::get_if<StatusOperation>(&operation)) {
                GetCounters().priorities[static_cast<std::size_t>(description.description.priority + 1)].statusCount.fetch_add(1, std::memory_order_relaxed);
                status->Complete(groupResult);
                groupResult = 0;
            } else if (auto* signal = std::get_if<SignalOperation>(&operation)) {
                GetCounters().priorities[static_cast<std::size_t>(description.description.priority + 1)].fenceCount.fetch_add(1, std::memory_order_relaxed);
                auto result = signal->Complete();
                if (result < 0) failure = result;
            } else if (auto* event = std::get_if<EventOperation>(&operation)) {
                if (!SetEvent(event->event.Get())) Windows::ThrowLastError("SetEvent DirectStorage queue");
            }
        } catch (const std::system_error& error) {
            failure = std::bit_cast<std::int32_t>(0x80070000U | (static_cast<std::uint32_t>(error.code().value()) & 0xFFFF));
        } catch (const std::bad_alloc&) {
            failure = std::bit_cast<std::int32_t>(std::uint32_t{0x8007000E});
        } catch (...) {
            failure = std::bit_cast<std::int32_t>(std::uint32_t{0x80004005});
        }
        if (failure != 0) {
            if (groupResult == 0) groupResult = failure;
            recordError(operation, failure);
        }
        operation.emplace<std::monostate>();
        lock.lock();
    }
}
