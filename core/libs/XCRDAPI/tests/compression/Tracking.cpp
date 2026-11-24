#include "Tests.hpp"
#include "../../src/metrics/Counters.hpp"
#include "../../src/metrics/Memory.hpp"
#include "../../src/compression/ZlibDecoder.hpp"

#include <limits>
#include <thread>

namespace {

class FailingObject : public AnyBox::Storage::TrackedObject {
public:
    FailingObject() { throw std::runtime_error("Tracked constructor failure"); }
};

}

void AnyBox::Tests::CheckTrackedMemory() {
    auto& counters = Storage::GetCounters();
    auto baseline = counters.currentMemory.load();
    {
        auto buffer = Storage::MakeTrackedArray<std::byte>(1234);
        Require(counters.currentMemory.load() == baseline + 1234, "Storage buffer allocation was not measured");
        {
            Storage::TrackedVector<std::uint64_t> values;
            values.reserve(37);
            Require(counters.currentMemory.load() >= baseline + 1234 + 37 * sizeof(std::uint64_t), "Storage vector allocation was not measured");
        }
        Require(counters.currentMemory.load() == baseline + 1234, "Storage vector allocation was not released");
    }
    Require(counters.currentMemory.load() == baseline, "Storage buffer allocation was not released");
    ExpectException<std::runtime_error>([] { auto object = std::make_unique<FailingObject>(); });
    Require(counters.currentMemory.load() == baseline, "Failed storage construction leaked its memory charge");
    {
        Storage::ZlibDecoder decoder;
        Require(counters.currentMemory.load() > baseline, "Codec workspace was not measured");
    }
    Require(counters.currentMemory.load() == baseline, "Codec workspace memory was not released");
    ExpectException<std::bad_array_new_length>([] { Storage::MakeTrackedArray<std::uint64_t>(std::numeric_limits<std::size_t>::max()); });
    Require(counters.currentMemory.load() == baseline, "Failed array allocation changed the storage memory charge");

    std::vector<std::jthread> threads;
    for (std::uint32_t index = 0; index < 4; ++index) {
        threads.emplace_back([] {
            for (std::uint32_t repeat = 0; repeat < 1000; ++repeat) {
                Storage::TrackedVector<std::byte> buffer(1024);
                Storage::TrackedString<char> name(127, 'x');
            }
        });
    }
    threads.clear();
    Require(counters.currentMemory.load() == baseline, "Concurrent storage allocations leaked their memory charges");
    Require(counters.maximumMemory.load() >= baseline + 1234 + 37 * sizeof(std::uint64_t), "Storage peak memory was not retained");
}
