#include "Configuration.hpp"

#include <algorithm>
#include <exception>

void AnyBox::Storage::WorkerConfiguration::Register(HANDLE thread) {
    std::lock_guard lock(mutex);
    if (affinity != 0 && !SetThreadAffinityMask(thread, static_cast<DWORD_PTR>(affinity))) Windows::ThrowLastError("SetThreadAffinityMask DirectStorage worker");
    threads.push_back(thread);
}

void AnyBox::Storage::WorkerConfiguration::Unregister(HANDLE thread) noexcept {
    std::lock_guard lock(mutex);
    auto entry = std::find(threads.begin(), threads.end(), thread);
    if (entry == threads.end()) std::terminate();
    threads.erase(entry);
    if (threads.empty()) {
        TrackedVector<HANDLE> empty;
        threads.swap(empty);
    }
}

AnyBox::Storage::WorkerConfiguration& AnyBox::Storage::GetWorkerConfiguration() {
    static WorkerConfiguration configuration;
    return configuration;
}

AnyBox::Storage::WorkerRegistration::WorkerRegistration() {
    HANDLE duplicate = nullptr;
    auto process = GetCurrentProcess();
    if (!DuplicateHandle(process, GetCurrentThread(), process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) Windows::ThrowLastError("DuplicateHandle DirectStorage worker");
    thread = Handle(duplicate);
    GetWorkerConfiguration().Register(thread.Get());
}

AnyBox::Storage::WorkerRegistration::~WorkerRegistration() {
    GetWorkerConfiguration().Unregister(thread.Get());
}
