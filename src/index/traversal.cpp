#include "myheader.h"

void traverseFd(int parent_fd, const char *name)
{
    // Open the subdirectory relative to parent fd
    int fd = openat(parent_fd, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        return;

    DIR *dir = fdopendir(fd); // fdopendir takes ownership of fd
    if (!dir)
    {
        close(fd);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        const char *entryName = entry->d_name;

        // Skip . and ..
        if (strcmp(entryName, ".") == 0 || strcmp(entryName, "..") == 0)
            continue;

        // Construct fd-relative full path for indexing queue
        std::string fullPath = std::string(name) + "/" + entryName;
        app.indexing.indexQueue.push(fullPath);

        // Recurse if directory
        if (entry->d_type == DT_DIR)
        {
            traverseFd(fd, entryName);
        }
    }

    closedir(dir); // closes fd automatically
}

void traverse(const std::string &rootStr)
{
    // Open root directory
    int fd = open(rootStr.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        return;

    DIR *dir = fdopendir(fd);
    if (!dir)
    {
        close(fd);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        const char *entryName = entry->d_name;

        // Skip . and ..
        if (strcmp(entryName, ".") == 0 || strcmp(entryName, "..") == 0)
            continue;

        std::string fullPath = rootStr + "/" + entryName;
        app.indexing.indexQueue.push(fullPath);

        if (entry->d_type == DT_DIR)
        {
            traverseFd(fd, entryName);
        }
    }

    closedir(dir); // closes root fd
}