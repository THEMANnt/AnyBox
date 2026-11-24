#ifndef ANYBOX_STORAGE_STATUS_ENTRY_HPP
#define ANYBOX_STORAGE_STATUS_ENTRY_HPP

#include <atomic>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace AnyBox::Storage {

inline constexpr std::uint32_t PendingResult = 0x8000000A;

struct StatusTicket {
    std::uint32_t generation;
    std::uint32_t previousResult;
};

class StatusEntry {
public:
    StatusTicket Begin() {
        auto value = state.load(std::memory_order_acquire);
        for (;;) {
            auto result = static_cast<std::uint32_t>(value);
            auto generation = static_cast<std::uint32_t>(value >> 32);
            if (result == PendingResult) throw std::logic_error("DirectStorage status is already pending");
            if (generation == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("DirectStorage status generation exhausted");
            auto next = (static_cast<std::uint64_t>(generation + 1) << 32) | PendingResult;
            if (state.compare_exchange_weak(value, next, std::memory_order_acq_rel)) return {generation + 1, result};
        }
    }
    void Complete(StatusTicket ticket, std::int32_t result) {
        auto bits = std::bit_cast<std::uint32_t>(result);
        if (bits == PendingResult) throw std::invalid_argument("Pending is not a completion result");
        publish(ticket, bits);
    }
    void Cancel(StatusTicket ticket) {
        publish(ticket, ticket.previousResult);
    }
    std::int32_t GetResult() const {
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(state.load(std::memory_order_acquire)));
    }
    bool IsComplete() const {
        return static_cast<std::uint32_t>(state.load(std::memory_order_acquire)) != PendingResult;
    }

private:
    void publish(StatusTicket ticket, std::uint32_t result) {
        auto expected = (static_cast<std::uint64_t>(ticket.generation) << 32) | PendingResult;
        auto desired = (static_cast<std::uint64_t>(ticket.generation) << 32) | result;
        if (!state.compare_exchange_strong(expected, desired, std::memory_order_acq_rel)) {
            throw std::logic_error("DirectStorage status completion is stale or duplicated");
        }
    }
    std::atomic<std::uint64_t> state{0};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

}

#endif
