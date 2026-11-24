#pragma once

#include "Tests.hpp"
#include <anybox/relinker/Lowering.hpp>

namespace AnyBox::Relinker::Tests {
struct InsertState {
    std::array<std::array<std::uint64_t, 2>, 16> values{};
    std::array<std::array<std::uint64_t, 2>, 10> saved{};
    std::uint64_t before = 0;
    std::uint64_t after = 0;
    std::uint64_t requested = 0;
};

class InsertFixture {
public:
    InsertFixture();
    ~InsertFixture();
    InsertFixture(const InsertFixture&) = delete;
    InsertFixture& operator=(const InsertFixture&) = delete;
    void Run(const LoweredFunction& function, InsertState& state);
    void RunAt(void* target, InsertState& state);

private:
    void* memory = nullptr;
    void invoke(InsertState& state);
};

DecodedInstruction InsertInstruction(unsigned destination, unsigned source, unsigned length, unsigned index);
void CheckInsertUnwind(const LoweredFunction& function);
}
