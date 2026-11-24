#include <d3d12_xs.hpp>
#include <xmem.hpp>
#include <anybox/platform/Windows.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const TException&) { return; }
    throw std::runtime_error("Invalid graphics memory configuration was accepted");
}

void CheckMemory(std::uintptr_t address, std::size_t size) {
    auto* bytes = reinterpret_cast<std::byte*>(address);
    Require(bytes && address % 0x10000 == 0, "Graphics service memory is null or misaligned");
    Require(bytes[0] == std::byte{} && bytes[size - 1] == std::byte{}, "Graphics service memory was not initialized");
    bytes[size - 1] = std::byte{0x73};
    MEMORY_BASIC_INFORMATION information{};
    Require(VirtualQuery(bytes + size - 1, &information, sizeof(information)) != 0, "Graphics service memory is not mapped");
    Require(information.State == MEM_COMMIT && information.Protect == PAGE_READWRITE, "Graphics service memory is not writable committed storage");
}
}

int main(int count, char** arguments) {
    try {
        XMemWorkingSetStatistics statistics{};
        XMemGetWorkingSetStatistics(0x20000000, &statistics);
        Require(statistics.gpuPageTableSize == 0 && statistics.framePreserveBufferSize == 0, "Unconfigured graphics statistics are not empty");
        Reject<std::invalid_argument>([] { D3D12ConfigureVirtualMemoryX(nullptr); });
        D3D12VirtualMemoryConfiguration invalid{};
        invalid.flags = 2;
        Reject<std::invalid_argument>([&] { D3D12ConfigureVirtualMemoryX(&invalid); });
        invalid.flags = 0;
        invalid.pageTablePageCount = 34;
        Reject<std::invalid_argument>([&] { D3D12ConfigureVirtualMemoryX(&invalid); });
        invalid.pageTablePageCount = 1;
        invalid.flags = 1;
        Reject<std::runtime_error>([&] { D3D12ConfigureVirtualMemoryX(&invalid); });

        auto defaults = count > 1 && std::string_view(arguments[1]) == "--default";
        D3D12VirtualMemoryConfiguration configuration{};
        configuration.pageTablePageCount = defaults ? 0 : 4;
        configuration.framePreserveBufferSize = defaults ? 0 : 0x10000;
        configuration.reserved = 0x7359;
        Require(D3D12ConfigureVirtualMemoryX(&configuration) == 0x10000000, "Graphics memory configuration returned an incorrect status");
        Require(configuration.reserved == 0x7359, "Graphics memory configuration changed reserved input storage");
        auto pageTableBytes = defaults ? 0x1400000u : 0x1000000u;
        CheckMemory(configuration.backendData[0], pageTableBytes);
        if (defaults) Require(configuration.backendData[1] == 0, "Absent frame preserve memory was allocated");
        else CheckMemory(configuration.backendData[1], 0x10000);
        XMemGetWorkingSetStatistics(0x20000000, &statistics);
        Require(statistics.gpuPageTableSize == pageTableBytes, "Graphics page table budget was not registered with XMem");
        Require(statistics.framePreserveBufferSize == configuration.framePreserveBufferSize, "Frame preserve budget was not registered with XMem");

        auto repeated = configuration;
        repeated.backendData = {};
        Require(D3D12ConfigureVirtualMemoryX(&repeated) == 0x10000000 && repeated.backendData == configuration.backendData, "Repeated graphics configuration changed its backing storage");
        repeated.pageTablePageCount = 0;
        repeated.backendData = {};
        Require(D3D12ConfigureVirtualMemoryX(&repeated) == 0x10000000 && repeated.backendData == configuration.backendData, "Default graphics request did not retain the existing configuration");
        XMemGetWorkingSetStatistics(0x20000000, &statistics);
        Require(statistics.gpuPageTableSize == pageTableBytes, "Default graphics request changed the registered budget");
        repeated.pageTablePageCount = 2;
        Reject<std::logic_error>([&] { D3D12ConfigureVirtualMemoryX(&repeated); });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
