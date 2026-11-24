#pragma once

#include "PrivateData.hpp"
#include <atomic>

namespace AnyBox::Graphics {
class PrivateDataSlot {
public:
    ~PrivateDataSlot() { Reset(); }
    void Reset() { delete value.exchange(nullptr, std::memory_order_acq_rel); }
    PrivateData& Get() {
        auto* current = value.load(std::memory_order_acquire);
        if (current) return *current;
        auto created = std::make_unique<PrivateData>();
        if (value.compare_exchange_strong(current, created.get(), std::memory_order_acq_rel, std::memory_order_acquire)) return *created.release();
        return *current;
    }

private:
    std::atomic<PrivateData*> value{nullptr};
};
}
