#include "Tests.hpp"
#include "Fixtures.hpp"

namespace {
template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); }
    catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Unresolved PE control flow was accepted");
}
}

void AnyBox::Relinker::Tests::CheckControlFlow() {
    CpuState state;
    state.basic[3] = 1U << 19;
    Cpu cpu(state);
    auto bytes = Fixture(std::array<std::uint8_t, 6>{0xFF, 0x25, 0x1A, 0x10, 0, 0});
    Put<std::uint64_t>(bytes, 1056, 0x140001020ULL);
    const std::array<std::uint8_t, 6> destination{0x66, 0x0F, 0xAE, 0x30, 0xC3, 0xCC};
    std::memcpy(bytes.data() + 544, destination.data(), destination.size());
    Image image(bytes);
    Require(Relink(image, cpu).writebackAddresses == std::vector<std::uint32_t>{0x1020}, "Indirect read-only target was not analyzed and lowered");
    Put<std::uint32_t>(bytes, 0x1D4, 0xC0000040);
    Reject([&] { Image writable(bytes); Relink(writable, cpu); });
    auto unknown = Fixture(std::array<std::uint8_t, 2>{0xFF, 0xE0});
    Reject([&] { Image input(unknown); Relink(input, cpu); });

    auto imported = Fixture(std::array<std::uint8_t, 7>{0xFF, 0x15, 0x9A, 0x10, 0, 0, 0xC3});
    Put<std::uint32_t>(imported, 0x1B8, 512);
    Put<std::uint32_t>(imported, 0x110, 0x2040);
    Put<std::uint32_t>(imported, 0x114, 40);
    Put<std::uint32_t>(imported, 1088, 0x2080);
    Put<std::uint32_t>(imported, 1100, 0x20C0);
    Put<std::uint32_t>(imported, 1104, 0x20A0);
    Put<std::uint64_t>(imported, 1152, 0x8000000000000001ULL);
    Put<std::uint64_t>(imported, 1184, 0x8000000000000001ULL);
    Image external(imported);
    Require(external.ImportSlots().contains(0x20A0) && Relink(external, cpu).instructionCount == 2, "Imported indirect call was not recognized as an external transfer");
    auto delayed = imported;
    Put<std::uint64_t>(delayed, 0x110, 0);
    Put<std::uint32_t>(delayed, 0x170, 0x2040);
    Put<std::uint32_t>(delayed, 0x174, 64);
    std::memset(delayed.data() + 1088, 0, 64);
    Put<std::uint32_t>(delayed, 1088, 1);
    Put<std::uint32_t>(delayed, 1092, 0x20C0);
    Put<std::uint32_t>(delayed, 1100, 0x20A0);
    Put<std::uint32_t>(delayed, 1104, 0x2080);
    Image delayImage(delayed);
    Require(delayImage.ImportSlots().contains(0x20A0) && Relink(delayImage, cpu).instructionCount == 2, "Delay-imported indirect call was not recognized as an external transfer");
    Put<std::uint32_t>(imported, 0x114, 20);
    Reject([&] { Image invalid(imported); invalid.ImportSlots(); });
}
