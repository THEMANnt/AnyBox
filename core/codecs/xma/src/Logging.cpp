#include "Logging.hpp"
#include "FFmpeg.hpp"
#include <cstdarg>
#include <mutex>

namespace {
thread_local bool* activeError = nullptr;

void Log(void* context, int level, const char* format, std::va_list arguments) {
    if (activeError && level <= AV_LOG_ERROR) *activeError = true;
    av_log_default_callback(context, level, format, arguments);
}
}

void AnyBox::Codecs::InitializeXmaLogging() {
    static std::once_flag initialized;
    std::call_once(initialized, [] { av_log_set_callback(Log); });
}

AnyBox::Codecs::XmaErrorScope::XmaErrorScope() noexcept : previous(activeError) { activeError = &failed; }
AnyBox::Codecs::XmaErrorScope::~XmaErrorScope() { activeError = previous; }
bool AnyBox::Codecs::XmaErrorScope::HasError() const noexcept { return failed; }
