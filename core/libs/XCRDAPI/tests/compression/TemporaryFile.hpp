#ifndef ANYBOX_STORAGE_COMPRESSED_TEMPORARY_FILE_HPP
#define ANYBOX_STORAGE_COMPRESSED_TEMPORARY_FILE_HPP

#include "Tests.hpp"

#include <array>

namespace AnyBox::Tests {

class CompressedFile {
public:
    explicit CompressedFile(std::span<const std::byte> data) {
        std::array<wchar_t, MAX_PATH> directory;
        AnyBox::Tests::Require(GetTempPathW(static_cast<DWORD>(directory.size()), directory.data()) != 0, "Compressed file temporary path query failed");
        AnyBox::Tests::Require(GetTempFileNameW(directory.data(), L"abz", 0, path.data()) != 0, "Compressed file creation failed");
        AnyBox::Storage::Handle handle(CreateFileW(path.data(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, 0, nullptr));
        if (handle.Get() == INVALID_HANDLE_VALUE) AnyBox::Windows::ThrowLastError("CreateFileW compressed test file");
        std::array<std::byte, 16> prefix{};
        DWORD written = 0;
        AnyBox::Tests::Require(WriteFile(handle.Get(), prefix.data(), static_cast<DWORD>(prefix.size()), &written, nullptr) != 0 && written == prefix.size(), "Compressed file prefix write failed");
        AnyBox::Tests::Require(WriteFile(handle.Get(), data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0 && written == data.size(), "Compressed file payload write failed");
    }
    ~CompressedFile() { DeleteFileW(path.data()); }
    const wchar_t* Path() const { return path.data(); }

private:
    std::array<wchar_t, MAX_PATH> path;
};

}

#endif
