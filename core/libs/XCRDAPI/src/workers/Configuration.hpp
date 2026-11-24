#ifndef ANYBOX_STORAGE_WORKER_CONFIGURATION_HPP
#define ANYBOX_STORAGE_WORKER_CONFIGURATION_HPP

#include "../common/Handle.hpp"
#include "../metrics/Memory.hpp"

#include <cstdint>
#include <mutex>

namespace AnyBox::Storage {

class WorkerConfiguration {
public:
    void SetAffinity(std::uint64_t affinity);
    void Register(HANDLE thread);
    void Unregister(HANDLE thread) noexcept;

private:
    std::mutex mutex;
    std::uint64_t affinity = 0;
    TrackedVector<HANDLE> threads;
};

class WorkerRegistration {
public:
    WorkerRegistration();
    WorkerRegistration(const WorkerRegistration&) = delete;
    WorkerRegistration& operator=(const WorkerRegistration&) = delete;
    ~WorkerRegistration();

private:
    Handle thread;
};

WorkerConfiguration& GetWorkerConfiguration();

}

#endif
