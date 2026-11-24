#include "Tests.hpp"

#include <system_error>

void AnyBox::Tests::CheckTransfer() {
    Require(XMemIsToolsMemAvailable() == 0, "Host incorrectly reports development-kit memory");
    for (auto direction : {1U, 2U}) {
        for (std::size_t size : {std::size_t{0}, std::size_t{1}, std::size_t{0x200000}, std::size_t{0x1000000}}) {
            bool caught = false;
            try {
                XMemTransferMemory(direction, size);
            } catch (const std::system_error& error) {
                caught = error.code().category() == std::system_category() && error.code().value() == ERROR_NOT_SUPPORTED;
            }
            Require(caught, "Tools memory transfer did not report the unsupported host resource");
        }
    }
    Require(XMemTransferMemory(0, 0) == 1, "Zero-sized system memory transfer failed");
    Require(XMemTransferMemory(0xFFFFFFFF, 0) == 1, "Zero-sized memory transfer changed its original no-op behavior");
    ExpectException<std::runtime_error>([] { XMemTransferMemory(0, 1); });
    ExpectException<std::invalid_argument>([] { XMemTransferMemory(0xFFFFFFFF, 1); });
}
