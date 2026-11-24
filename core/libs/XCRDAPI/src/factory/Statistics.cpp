#include "Factory.hpp"
#include "../metrics/Counters.hpp"

#include <stdexcept>

void AnyBox::Storage::Factory::GetStats(StorageStatistics* statistics) {
    if (!statistics) throw std::invalid_argument("DirectStorage statistics output is null");
    *statistics = GetStatistics();
}
