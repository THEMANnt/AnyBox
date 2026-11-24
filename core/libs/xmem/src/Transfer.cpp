#include <xmem.hpp>
#include <anybox/Unimplemented.hpp>
#include <anybox/platform/Windows.hpp>

#include <stdexcept>
#include <system_error>

std::int32_t XMemTransferMemory(std::uint32_t memoryPool, std::size_t size) noexcept(false) {
    if (memoryPool == 1 || memoryPool == 2) {
        throw std::system_error(ERROR_NOT_SUPPORTED, std::system_category(), "XMem tools memory is not available on this host");
    }
    if (size == 0) return 1;
    if (memoryPool > 2) throw std::invalid_argument("Unsupported XMem memory transfer direction");
    ANYBOX_NOT_IMPLEMENTED();
}
