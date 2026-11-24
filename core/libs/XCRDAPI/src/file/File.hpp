#ifndef ANYBOX_STORAGE_FILE_HPP
#define ANYBOX_STORAGE_FILE_HPP

#include "../common/ReferenceCount.hpp"
#include "../metrics/Memory.hpp"
#include "../metrics/Lifetime.hpp"
#include "ReadLease.hpp"

#include <StorageInterfaces.hpp>
#include "../staging/Pool.hpp"
#include <anybox/platform/Windows.hpp>

#include <condition_variable>
#include <mutex>
#include <string>

namespace AnyBox::Storage {

class File final : public IStorageFile, public TrackedObject {
public:
    explicit File(const wchar_t* path);
    ~File();
    std::int32_t QueryInterface(const Guid& interfaceId, void** object) override;
    std::uint32_t AddRef() override;
    std::uint32_t Release() override;
    void Close() override;
    void* GetHandle() override;
    ReadLease AcquireRead();

private:
    friend class ReadLease;
    void finishRead();
    StorageObjectRegistration registration;
    FileStatistics statisticsRegistration;
    ReferenceCount references{1};
    TrackedString<wchar_t> path;
    std::mutex mutex;
    std::condition_variable readsFinished;
    std::size_t pendingReads = 0;
    bool closing = false;
    HANDLE handle = INVALID_HANDLE_VALUE;
};

}

#endif
