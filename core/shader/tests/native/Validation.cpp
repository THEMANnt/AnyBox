#include "../../src/backend/Validation.hpp"
#include "NativeFixtures.hpp"
#include <vector>

void CheckSpirvValidation(std::span<const std::uint32_t> code) {
    using namespace Fixtures;
    using AnyBox::Shader::ValidateSpirv;
    ValidateSpirv(code, 0x402000);
    Reject<std::runtime_error>([&] { ValidateSpirv({}, 0x402000); });
    Reject<std::invalid_argument>([&] { ValidateSpirv(code, 0); });
    Reject<std::runtime_error>([&] { ValidateSpirv(code, 0x400000); });
    std::vector<std::uint32_t> invalid(code.begin(), code.end());
    invalid[0] = 0;
    Reject<std::runtime_error>([&] { ValidateSpirv(invalid, 0x402000); });
    invalid.assign(code.begin(), code.end());
    invalid.pop_back();
    Reject<std::runtime_error>([&] { ValidateSpirv(invalid, 0x402000); });
    invalid.assign(code.begin(), code.end());
    bool changed = false;
    for (std::size_t offset = 5; offset < invalid.size();) {
        auto words = invalid[offset] >> 16;
        Require(words != 0 && words <= invalid.size() - offset);
        if ((invalid[offset] & 65535) == 15) {
            Require(words >= 3);
            invalid[offset + 2] = 0;
            changed = true;
            break;
        }
        offset += words;
    }
    Require(changed);
    Reject<std::runtime_error>([&] { ValidateSpirv(invalid, 0x402000); });
}
