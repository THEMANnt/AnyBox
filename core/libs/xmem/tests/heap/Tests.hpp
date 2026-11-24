#ifndef ANYBOX_XMEM_HEAP_TESTS_HPP
#define ANYBOX_XMEM_HEAP_TESTS_HPP

#include <xmem.hpp>

#include <stdexcept>

namespace AnyBox::Tests {

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void ExpectException(TCallable&& callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected XMem heap error was not raised");
}

void CheckDefaultHeap();
void CheckGraphicsHeap();
void CheckHeapErrors();
void CheckHeapConcurrency();

}

#endif
