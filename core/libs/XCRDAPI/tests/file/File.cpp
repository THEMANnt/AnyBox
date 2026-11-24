#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <filesystem>

namespace {

class TemporaryFile {
public:
    TemporaryFile() {
        std::array<wchar_t, MAX_PATH> folder{};
        AnyBox::Tests::Require(GetTempPathW(static_cast<DWORD>(folder.size()), folder.data()) != 0, "Temporary folder query failed");
        AnyBox::Tests::Require(GetTempFileNameW(folder.data(), L"abx", 0, path.data()) != 0, "Temporary file creation failed");
        auto handle = CreateFileW(path.data(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) AnyBox::Windows::ThrowLastError("CreateFileW test data");
        std::array<std::byte, 4096> data;
        for (std::size_t i = 0; i < data.size(); ++i) data[i] = std::byte(i % 251);
        DWORD written = 0;
        auto result = WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
        CloseHandle(handle);
        AnyBox::Tests::Require(result != 0 && written == data.size(), "Temporary data write failed");
    }
    ~TemporaryFile() { DeleteFileW(path.data()); }
    const wchar_t* Path() const { return path.data(); }

private:
    std::array<wchar_t, MAX_PATH> path{};
};

}

void AnyBox::Tests::CheckFiles() {
    TemporaryFile source;
    auto factory = GetFactory();
    void* output = nullptr;
    Require(factory->OpenFile(source.Path(), Storage::FileId, &output) == 0 && output, "DirectStorage file open failed");
    Object<IStorageFile> file(static_cast<IStorageFile*>(output));
    auto first = file->GetHandle();
    auto second = file->GetHandle();
    Require(first != second, "DirectStorage returned a shared handle without ownership transfer");
    LARGE_INTEGER size{};
    Require(GetFileSizeEx(first, &size) != 0 && size.QuadPart == 4096, "DirectStorage file size changed");
    Require(CloseHandle(first) != 0, "DirectStorage first duplicate could not close");
    file->Close();
    file->Close();
    std::array<std::byte, 257> buffer{};
    OVERLAPPED operation{};
    operation.Offset = 197;
    operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Require(operation.hEvent != nullptr, "Read test event creation failed");
    DWORD read = 0;
    auto completed = ReadFile(second, buffer.data(), static_cast<DWORD>(buffer.size()), &read, &operation);
    if (!completed) Require(GetLastError() == ERROR_IO_PENDING, "DirectStorage duplicate read failed");
    auto finished = GetOverlappedResult(second, &operation, &read, TRUE);
    CloseHandle(operation.hEvent);
    CloseHandle(second);
    Require(finished != 0 && read == buffer.size(), "DirectStorage duplicate stopped working after file close");
    for (std::size_t i = 0; i < buffer.size(); ++i) Require(buffer[i] == std::byte((i + 197) % 251), "DirectStorage file contents changed");
    ExpectException<std::logic_error>([&] { file->GetHandle(); });
    output = file.get();
    ExpectException<std::invalid_argument>([&] { factory->OpenFile(source.Path(), {}, &output); });
    Require(output == file.get(), "Failed DirectStorage open changed its output");
    auto missing = std::filesystem::path(source.Path());
    missing += L".missing";
    ExpectException<std::system_error>([&] { factory->OpenFile(missing.c_str(), Storage::FileId, &output); });
    ExpectException<std::invalid_argument>([&] { factory->OpenFile(nullptr, Storage::FileId, &output); });
}
