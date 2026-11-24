#include "Tests.hpp"
#include "Fixtures.hpp"
#include <anybox/relinker/Relink.hpp>
#include <algorithm>
#include <cstring>

namespace {
AnyBox::Relinker::RelinkResult LowerCacheWriteback(AnyBox::Relinker::Image& image) {
    AnyBox::Relinker::CpuState state;
    state.basic[3] = 1U << 19;
    return AnyBox::Relinker::Relink(image, AnyBox::Relinker::Cpu(state), true);
}

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); }
    catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Malformed or ambiguous PE input was accepted");
}
}

void AnyBox::Relinker::Tests::CheckImages() {
    const std::array<std::uint8_t, 9> code{0x49, 0x89, 0xC9, 0x66, 0x41, 0x0F, 0xAE, 0x31, 0xC3};
    auto original = Fixture(code);
    Image image(original);
    auto result = LowerCacheWriteback(image);
    Require(result.writebackAddresses == std::vector<std::uint32_t>{0x1003}, "PE pass did not find the cache instruction at its actual boundary");
    Require(image.GetBytes().size() == original.size(), "PE lowering moved file data");
    Require(std::ranges::equal(image.GetBytes().subspan(1024), std::span(original).subspan(1024)), "PE lowering changed its unwind data");
    for (std::size_t index = 0; index < original.size(); ++index) {
        if (index != 515 && index != 519) Require(image.GetBytes()[index] == original[index], "PE lowering changed bytes outside the cache instruction");
    }
    Require(image.GetBytes()[515] == std::byte{0x40} && image.GetBytes()[519] == std::byte{0x39}, "PE cache replacement encoding changed");
    Require(LowerCacheWriteback(image).writebackAddresses.empty(), "PE cache lowering is not idempotent");

    CpuState nativeState;
    nativeState.structured[1] = 1U << 24;
    Image nativeImage(original);
    Require(Relink(nativeImage, Cpu(nativeState)).writebackAddresses.empty() && std::ranges::equal(nativeImage.GetBytes(), original), "Supported CLWB was rewritten without a request");
    Reject([&] { Image invalid(original); Relink(invalid, Cpu(nativeState), true); });
    Reject([&] { Image invalid(original); Relink(invalid, Cpu(CpuState{})); });

    const std::array<std::uint8_t, 12> branch{0x85, 0xC0, 0x74, 0x01, 0xC3, 0x66, 0x0F, 0x78, 0xC0, 1, 1, 0xC3};
    Reject([&] { Image invalid(Fixture(branch)); Relink(invalid, Cpu(CpuState{})); });
    auto fallthrough = branch;
    fallthrough[3] = 7;
    fallthrough[4] = 0x90;
    Reject([&] { Image invalid(Fixture(fallthrough)); Relink(invalid, Cpu(CpuState{})); });

    auto signedBytes = original;
    signedBytes.resize(1568, std::byte{0xA7});
    Put<std::uint32_t>(signedBytes, 0xD8, 0x12345678);
    Put<std::uint32_t>(signedBytes, 0x128, 1536);
    Put<std::uint32_t>(signedBytes, 0x12C, 32);
    Image signedImage(signedBytes);
    LowerCacheWriteback(signedImage);
    Require(Read<std::uint32_t>(signedImage.GetBytes(), 0xD8) == 0 && Read<std::uint64_t>(signedImage.GetBytes(), 0x128) == 0, "PE lowering retained an invalid checksum or certificate directory");
    Require(std::ranges::equal(signedImage.GetBytes().subspan(1024), std::span(signedBytes).subspan(1024)), "PE header finalization changed section data or its overlay");

    const std::array<std::uint8_t, 11> immediate{0x48, 0xB8, 0x66, 0x41, 0x0F, 0xAE, 0x31, 0, 0, 0, 0xC3};
    auto data = Fixture(immediate);
    Image literal(data);
    Require(LowerCacheWriteback(literal).writebackAddresses.empty() && std::ranges::equal(literal.GetBytes(), data), "PE pass mistook immediate data for a cache instruction");

    auto corrupt = original;
    Put<std::uint32_t>(corrupt, 0x19C, 0xFFFFFFF0);
    Reject([&] { Image invalid(corrupt); });
    corrupt = original;
    Put<std::uint32_t>(corrupt, 0x124, 13);
    Reject([&] { Image invalid(corrupt); LowerCacheWriteback(invalid); });
    corrupt = original;
    corrupt[512] = std::byte{0xC3};
    Reject([&] { Image ambiguous(corrupt); LowerCacheWriteback(ambiguous); });

    auto loader = Fixture(std::array<std::uint8_t, 1>{0xC3});
    Put<std::uint32_t>(loader, 0x1B8, 512);
    Put<std::uint32_t>(loader, 0x150, 0x2040);
    Put<std::uint32_t>(loader, 0x154, 40);
    Put<std::uint64_t>(loader, 1088 + 24, 0x140002080ULL);
    Put<std::uint64_t>(loader, 1152, 0x140001020ULL);
    loader[544] = std::byte{0xC3};
    Require(Image(loader).EntryPoints() == std::vector<std::uint32_t>{0x1000, 0x1020}, "PE TLS callback was not included as a code root");
    Put<std::uint64_t>(loader, 1152, 0x140002010ULL);
    Reject([&] { Image invalid(loader); invalid.EntryPoints(); });

    loader = Fixture(std::array<std::uint8_t, 1>{0xC3});
    Put<std::uint32_t>(loader, 0x1B8, 512);
    Put<std::uint32_t>(loader, 0x158, 0x2040);
    Put<std::uint32_t>(loader, 0x15C, 148);
    Put<std::uint32_t>(loader, 1088, 148);
    Put<std::uint64_t>(loader, 1088 + 128, 0x140002100ULL);
    Put<std::uint64_t>(loader, 1088 + 136, 2);
    Put<std::uint32_t>(loader, 1088 + 144, 0x10000400);
    Put<std::uint32_t>(loader, 1280, 0x1020);
    Put<std::uint32_t>(loader, 1285, 0x1030);
    Require(Image(loader).EntryPoints() == std::vector<std::uint32_t>{0x1000, 0x1020, 0x1030}, "PE guard table metadata stride lost an indirect function target");
    Put<std::uint64_t>(loader, 1088 + 136, UINT64_MAX);
    Reject([&] { Image invalid(loader); invalid.EntryPoints(); });
}
