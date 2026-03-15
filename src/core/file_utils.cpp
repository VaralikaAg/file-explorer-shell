#include "myheader.h"

bool isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}

bool isValidDirectory(const string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool isRegularFile(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

bool isDirectoryPath(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool isReadableFile(const string &filepath) {
    return (access(filepath.c_str(), R_OK) == 0);
}

bool isBinaryFile(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);

    if (!file)
        return false; // cannot open → treat as non-binary

    std::vector<char> buffer(512);
    file.read(buffer.data(), buffer.size());
    std::streamsize bytesRead = file.gcount();

    if (bytesRead <= 0)
        return false;

    for (std::streamsize i = 0; i < bytesRead; i++)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);

        // null byte → strong binary indicator
        if (c == '\0')
            return true;

        // non-printable and not whitespace
        if (!std::isprint(c) && !std::isspace(c))
            return true;
    }

    return false;
}