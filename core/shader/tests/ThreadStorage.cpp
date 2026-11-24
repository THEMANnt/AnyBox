#include <pthread.h>
#include <array>
#include <atomic>
#include <exception>
#include <memory>
#include <thread>

namespace {
struct Value {
    std::atomic<unsigned>* destroyed;
    unsigned number;
};

void Destroy(void* data) {
    std::unique_ptr<Value> value(static_cast<Value*>(data));
    ++*value->destroyed;
}
}

void CheckThreadStorage() {
    pthread_key_t key{};
    if (pthread_key_create(&key, Destroy) != 0) throw std::runtime_error("Shader thread storage allocation failed");
    auto free = [key](void*) { if (pthread_key_delete(key) != 0) std::terminate(); };
    std::unique_ptr<void, decltype(free)> owned(reinterpret_cast<void*>(1), free);
    std::atomic<unsigned> destroyed{0};
    std::array<std::exception_ptr, 2> errors{};
    auto run = [&](unsigned index) {
        try {
            if (pthread_getspecific(key)) throw std::runtime_error("Shader thread storage leaked across threads");
            auto value = std::make_unique<Value>(&destroyed, index);
            pthread_setspecific(key, value.get());
            if (pthread_getspecific(key) != value.get()) throw std::runtime_error("Shader thread storage did not retain its value");
            pthread_setspecific(key, nullptr);
            if (pthread_getspecific(key)) throw std::runtime_error("Shader thread storage did not clear its value");
            pthread_setspecific(key, value.get());
            value.release();
        } catch (...) { errors[index] = std::current_exception(); }
    };
    std::jthread first(run, 0);
    std::jthread second(run, 1);
    first.join();
    second.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    if (destroyed != 2 || pthread_getspecific(key)) throw std::runtime_error("Shader thread storage did not destroy values at thread exit");
}
