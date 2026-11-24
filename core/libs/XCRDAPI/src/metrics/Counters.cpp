#include "Counters.hpp"

AnyBox::Storage::Counters& AnyBox::Storage::GetCounters() {
    static Counters counters;
    return counters;
}
