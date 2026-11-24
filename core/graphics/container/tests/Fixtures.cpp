#include "Fixtures.hpp"
#include <algorithm>

std::vector<std::byte> Fixtures::MakeProgram() {
    std::vector<std::byte> data(32);
    Set(data, 0, 0x50065);
    Set(data, 4, 8);
    Set(data, 8, 0x4C495844);
    Set(data, 12, 0x105);
    Set(data, 16, 16);
    Set(data, 20, 8);
    Set(data, 24, 0xDEC04342);
    Set(data, 28, 3);
    return data;
}

std::vector<std::byte> Fixtures::MakeOptions() {
    std::vector<std::byte> data(26);
    Set(data, 0, 0x706F6278);
    Set(data, 4, 2);
    Set(data, 8, 627);
    Set(data, 12, 1);
    data[16] = std::byte{'1'};
    Set(data, 17, 677);
    Set(data, 21, 1);
    data[25] = std::byte{'0'};
    return data;
}

std::vector<std::byte> Fixtures::MakeXboxProgram() {
    auto program = MakeProgram();
    auto options = MakeOptions();
    std::vector<std::byte> data(136);
    std::ranges::copy(std::span(program).first(24), data.begin());
    Set(data, 4, static_cast<std::uint32_t>(data.size() / 4));
    Set(data, 16, 88);
    Set(data, 24, 0x54414458);
    Set(data, 28, 3);
    Set(data, 32, 36);
    Set(data, 36, 7);
    Set(data, 40, 72);
    Set(data, 44, static_cast<std::uint32_t>(program.size()));
    Set(data, 48, 10);
    Set(data, 52, 104);
    Set(data, 56, static_cast<std::uint32_t>(options.size()));
    Set(data, 60, 99);
    Set(data, 64, 132);
    Set(data, 68, 4);
    std::ranges::copy(program, data.begin() + 72);
    std::ranges::copy(options, data.begin() + 104);
    Set(data, 132, 0x12345678);
    return data;
}
