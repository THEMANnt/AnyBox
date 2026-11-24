#include "File.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <exception>
#include <stdexcept>

AnyBox::Storage::File::File(const wchar_t* path) {
    if (!path || *path == L'\0') throw std::invalid_argument("DirectStorage file path is empty");
    this->path = path;
    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
    if (handle == INVALID_HANDLE_VALUE) Windows::ThrowLastError("CreateFileW DirectStorage");
    statisticsRegistration.Open();
}

AnyBox::Storage::File::~File() {
    Close();
}

std::int32_t AnyBox::Storage::File::QueryInterface(const Guid& interfaceId, void** object) {
    if (!object) throw std::invalid_argument("DirectStorage interface output is null");
    if (interfaceId != UnknownId && interfaceId != FileId) throw std::invalid_argument("Unsupported DirectStorage file interface");
    AddRef();
    *object = static_cast<IStorageFile*>(this);
    return 0;
}

std::uint32_t AnyBox::Storage::File::AddRef() {
    return references.Add();
}

std::uint32_t AnyBox::Storage::File::Release() {
    auto count = references.Release();
    if (count == 0) delete this;
    return count;
}

void AnyBox::Storage::File::Close() {
    std::unique_lock lock(mutex);
    closing = true;
    readsFinished.wait(lock, [this] { return pendingReads == 0; });
    if (handle == INVALID_HANDLE_VALUE) return;
    if (!CloseHandle(handle)) Windows::ThrowLastError("CloseHandle DirectStorage file");
    handle = INVALID_HANDLE_VALUE;
    statisticsRegistration.Close();
}

void* AnyBox::Storage::File::GetHandle() {
    std::lock_guard lock(mutex);
    if (closing || handle == INVALID_HANDLE_VALUE) throw std::logic_error("DirectStorage file is closed");
    HANDLE duplicate;
    auto process = GetCurrentProcess();
    if (!DuplicateHandle(process, handle, process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) Windows::ThrowLastError("DuplicateHandle DirectStorage file");
    return duplicate;
}
