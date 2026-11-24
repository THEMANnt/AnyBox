#include "Tests.hpp"
#include "Fixtures.hpp"
#include <algorithm>

namespace {
template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); }
    catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Invalid PE lowering layout was accepted");
}
}

void AnyBox::Relinker::Tests::CheckPatching() {
    const std::array<std::uint8_t, 15> body{0x48, 0x83, 0xEC, 0x28, 0xF2, 0x0F, 0x78, 0xC0, 8, 24, 0x48, 0x83, 0xC4, 0x28, 0xC3};
    auto original = Fixture(body);
    const std::array<std::uint8_t, 8> unwind{1, 4, 1, 0, 4, 0x42, 0, 0};
    std::memcpy(original.data() + 1072, unwind.data(), unwind.size());
    original.resize(1600, std::byte{0xA7});
    CpuState state;
    state.basic[3] = 1U << 26;
    Cpu cpu(state);
    Image image(original);
    Require(image.GetStackAllocation(0x1000) == 0 && image.GetStackAllocation(0x1004) == 40, "PE stack allocation did not account for prolog progress");
    auto result = Relink(image, cpu);
    Require(result.insertAddresses == std::vector<std::uint32_t>{0x1004} && image.GetSections().size() == 3, "PE INSERTQ lowering did not add its helper section");
    const auto& section = image.GetSections().back();
    Require(section.address == 0x3000 && section.offset == 2048 && (section.flags & 0xE0000000) == 0x60000000, "PE helper section changed placement or writable/executable permissions");
    Require(std::ranges::equal(image.GetBytes().subspan(1024, original.size() - 1024), std::span(original).subspan(1024)), "PE helper placement changed original unwind data or its overlay");
    Require(std::ranges::equal(image.GetBytes().subspan(512, 4), std::span(original).subspan(512, 4)) && std::ranges::equal(image.GetBytes().subspan(522, 502), std::span(original).subspan(522, 502)), "PE helper placement moved or changed neighboring instructions");
    Decoder decoder;
    auto call = decoder.Decode(image.Code(0x1004));
    ZyanU64 target = 0;
    Require(call.native.mnemonic == ZYDIS_MNEMONIC_CALL && ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&call.native, &call.operands[0], image.GetBase() + 0x1004, &target)) && target == image.GetBase() + section.address, "PE helper call displacement is incorrect");
    Require(image.GetBytes()[521] == std::byte{0x90}, "PE helper call did not preserve the original instruction width");
    auto directory = Read<std::uint32_t>(image.GetBytes(), 0x120);
    Require(Read<std::uint32_t>(image.GetBytes(), 0x124) == 36 && std::ranges::equal(image.GetBytes().subspan(image.FileOffset(directory, 12), 12), std::span(original).subspan(1024, 12)), "PE helper placement did not preserve the original runtime function entry");
    auto rewritten = std::vector<std::byte>(image.GetBytes().begin(), image.GetBytes().end());
    Require(Relink(image, cpu).insertAddresses.empty() && std::ranges::equal(image.GetBytes(), rewritten), "PE INSERTQ lowering was not idempotent");
    CheckPatchedExecution(image, (0x123456789ABCDEF0ULL & ~0xFF000000ULL) | (0xF0ULL << 24));

    auto extract = Fixture(std::array<std::uint8_t, 15>{0x48, 0x83, 0xEC, 0x28, 0x66, 0x0F, 0x78, 0xC0, 27, 11, 0x48, 0x83, 0xC4, 0x28, 0xC3});
    std::memcpy(extract.data() + 1072, unwind.data(), unwind.size());
    Image extractImage(extract);
    auto extracted = Relink(extractImage, cpu);
    Require(extracted.extractAddresses == std::vector<std::uint32_t>{0x1004} && extracted.insertAddresses.empty(), "PE EXTRQ lowering did not report its original call site");
    CheckPatchedExecution(extractImage, (0x123456789ABCDEF0ULL >> 11) & 0x7FFFFFF);

    auto invalid = original;
    Put<std::uint32_t>(invalid, 0xD4, 472);
    Reject([&] { Image input(invalid); Relink(input, cpu); });
    invalid = original;
    Put<std::uint32_t>(invalid, 0xD0, 0x90000000);
    Reject([&] { Image input(invalid); Relink(input, cpu); });
    auto function = LowerImmediateInsert(decoder.Decode(std::as_bytes(std::span(body)).subspan(4)));
    std::array<LoweringSite, 2> duplicate{{{0x1004, 6, function}, {0x1004, 6, function}}};
    Image unchanged(original);
    Reject([&] { unchanged.AppendLowerings(duplicate); });
    Require(std::ranges::equal(unchanged.GetBytes(), original), "Rejected PE helper layout modified its image");

    auto leaf = Fixture(std::array<std::uint8_t, 7>{0xF2, 0x0F, 0x78, 0xC0, 8, 24, 0xC3});
    leaf[1072] = std::byte{1};
    Image leafImage(leaf);
    Relink(leafImage, cpu);
    auto helper = decoder.Decode(leafImage.Code(leafImage.GetSections().back().address));
    Require(helper.native.mnemonic == ZYDIS_MNEMONIC_LEA && helper.operands[1].mem.disp.value == -32, "PE leaf call-site lowering allocated an incorrectly aligned helper stack");
    CheckPatchedExecution(leafImage, (0x123456789ABCDEF0ULL & ~0xFF000000ULL) | (0xF0ULL << 24));
}
