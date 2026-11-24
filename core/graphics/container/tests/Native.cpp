#include "NativeFixtures.hpp"
#include <anybox/graphics/container/Native.hpp>
#include <anybox/graphics/container/Dxil.hpp>
#include <algorithm>
#include <array>

void CheckNative() {
    using namespace Fixtures;
    using namespace AnyBox::Graphics::Container;
    auto data = MakeNativeProgram();
    FssmView view(data);
    Require(view.GetWaveSize() == 32 && view.GetCodeSectionOffset() == 0x140 && view.GetCodeSectionSize() == 64);
    Require(view.GetCode().data() == data.data() + 0x168 && view.GetCode().size() == 24);
    auto other = data;
    Set(other, 0xD4, 0);
    Require(FssmView(other).GetWaveSize() == 64);
    for (std::size_t size = 0; size < data.size(); ++size) Reject<std::invalid_argument>([&] { FssmView rejected{std::span(data).first(size)}; });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 6>{{{0, 0}, {0xC0, 0}, {0xC0, 0x141}, {0xC4, 1}, {0xC8, 40}, {0xCC, 1}}}) {
        auto invalid = data;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { FssmView rejected(invalid); });
    }
    auto invalid = data;
    Set(invalid, 4, 0x102);
    Reject<std::runtime_error>([&] { FssmView rejected(invalid); });
    auto packed = PackNativeProgram(data);
    Require(UnpackXbch(packed) == data);
    for (std::size_t size = 0; size < packed.size(); ++size) Reject<std::invalid_argument>([&] { (void)UnpackXbch(std::span(packed).first(size)); });
    auto damaged = packed;
    Require(!damaged.empty());
    damaged.back() ^= std::byte{1};
    Reject<std::invalid_argument>([&] { (void)UnpackXbch(damaged); });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 3>{{{0, 0}, {8, 0}, {16, 0}}}) {
        damaged = packed;
        Set(damaged, offset, value);
        Reject<std::invalid_argument>([&] { (void)UnpackXbch(damaged); });
    }
    for (unsigned offset : {4u, 12u, 20u}) {
        damaged = packed;
        Set(damaged, offset, 1);
        Reject<std::runtime_error>([&] { (void)UnpackXbch(damaged); });
    }
    damaged = packed;
    damaged.push_back(std::byte{0});
    Set(damaged, 8, static_cast<std::uint32_t>(damaged.size() - 24));
    Reject<std::invalid_argument>([&] { (void)UnpackXbch(damaged); });
    auto second = PackNativeProgram(other);
    auto container = MakeNativeContainer(packed, second);
    auto programs = UnpackNativePrograms(container);
    Require(programs.size() == 2 && programs[0] == data && programs[1] == other);
    DxbcView outer(container);
    DxilView header(outer.FindPart(0x4C495844)->data);
    Require(!header.HasBitcode() && header.GetEmbeddedPartCount() == 3);
    Reject<std::runtime_error>([&] { (void)header.GetBitcode(); });
    Reject<std::invalid_argument>([&] { (void)header.FindEmbeddedPart(13); });
}
