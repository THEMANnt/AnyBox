#ifndef ANYBOX_STORAGE_STATISTICS_TESTS_HPP
#define ANYBOX_STORAGE_STATISTICS_TESTS_HPP

#include "../compression/Tests.hpp"

namespace AnyBox::Tests {

inline StorageStatistics ReadStatistics(IStorageFactory* factory) {
    StorageStatistics result{};
    factory->GetStats(&result);
    return result;
}

void CheckStatisticsLifetime();
void CheckStatisticsMemory();
void CheckStatisticsFile();
void CheckStatisticsFence();
void CheckStatisticsConcurrency();

}

#endif
