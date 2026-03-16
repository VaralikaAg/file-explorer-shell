#include "myheader.h"

bool isDirectory(const fs::path &path) noexcept
{
    std::error_code ec;
    return fs::is_directory(fs::symlink_status(path, ec));
}

bool isRegularFile(const fs::path &path) noexcept
{
    std::error_code ec;
    return fs::is_regular_file(fs::symlink_status(path, ec));
}

bool isReadable(const fs::path& path) noexcept {
    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();

    return (perms & fs::perms::owner_read) != fs::perms::none;
}

bool isBinaryFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    std::vector<char> buffer(512);
    file.read(buffer.data(), buffer.size());
    std::streamsize bytesRead = file.gcount();

    for (std::streamsize i = 0; i < bytesRead; i++) {
        unsigned char c = static_cast<unsigned char>(buffer[i]);

        if (c == '\0') return true;

        if (!std::isprint(c) && !std::isspace(c))
            return true;
    }
    return false;
}