#include "Configuration.hpp"
#include <xmem.hpp>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace {
std::mutex configurationMutex;
std::optional<XMemGraphicsMemoryConfiguration> graphicsConfiguration;
}

void XMemRegisterGraphicsMemoryConfiguration(const XMemGraphicsMemoryConfiguration* configuration) noexcept(false) {
    if (!configuration) throw std::invalid_argument("XMem graphics memory configuration is null");
    std::lock_guard lock(configurationMutex);
    if (!graphicsConfiguration) graphicsConfiguration = *configuration;
}

void AnyBox::XMem::GetGraphicsStatistics(XMemWorkingSetStatistics& statistics) {
    std::lock_guard lock(configurationMutex);
    if (!graphicsConfiguration) return;
    statistics.gpuPageTableSize = graphicsConfiguration->pageTableSize != 0 ? graphicsConfiguration->pageTableSize : 0x1400000;
    statistics.framePreserveBufferSize = graphicsConfiguration->framePreserveBufferSize;
}
