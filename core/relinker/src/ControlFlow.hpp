#pragma once

#include <anybox/relinker/Decoder.hpp>
#include <anybox/relinker/Image.hpp>

namespace AnyBox::Relinker {
bool FollowTransfer(const Image& image, const DecodedInstruction& instruction, std::uint32_t address, const std::unordered_set<std::uint32_t>& imports, std::vector<std::uint32_t>& entries);
}
