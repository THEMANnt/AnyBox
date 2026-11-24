#include <xmem.hpp>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        bool rejected = false;
        try { XMemRegisterGraphicsMemoryConfiguration(nullptr); } catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Null graphics configuration was accepted");
        XMemGraphicsMemoryConfiguration configuration{0, 0x12345, {0, 0}};
        XMemRegisterGraphicsMemoryConfiguration(&configuration);
        configuration.pageTableSize = 0x800000;
        configuration.framePreserveBufferSize = 7;
        XMemRegisterGraphicsMemoryConfiguration(&configuration);
        XMemWorkingSetStatistics statistics{};
        XMemGetWorkingSetStatistics(0x20000000, &statistics);
        if (statistics.gpuPageTableSize != 0x1400000 || statistics.framePreserveBufferSize != 0x12345) throw std::runtime_error("Graphics configuration registration did not retain the first configuration");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
