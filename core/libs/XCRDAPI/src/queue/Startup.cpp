#include "Queue.hpp"
#include "../workers/Configuration.hpp"

#include <exception>

void AnyBox::Storage::Queue::run() {
    try {
        WorkerRegistration workerRegistration;
        {
            std::lock_guard lock(mutex);
            started = true;
            wake.notify_all();
        }
        process();
    } catch (...) {
        std::lock_guard lock(mutex);
        if (started) std::terminate();
        startupError = std::current_exception();
        started = true;
        wake.notify_all();
    }
}
