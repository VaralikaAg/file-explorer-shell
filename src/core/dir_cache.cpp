#include "myheader.hpp"

void invalidateDirCache(const std::string &dir_path)
{
    auto it = app.cache.dir_cache.find(dir_path);
    if (it != app.cache.dir_cache.end())
        app.cache.dir_cache.erase(it);
}

std::vector<std::string> getDirectoryFiles(const fs::path &path)
{
    std::string dir_path = path.string();

    auto it = app.cache.dir_cache.find(dir_path);
    if (it != app.cache.dir_cache.end())
    {
        return it->second;
    }

    std::vector<std::string> temp_list;

    try
    {
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto &entry : fs::directory_iterator(path))
            {
                std::string name = entry.path().filename().string();
                if (name == "." || name == "..") continue;
                temp_list.push_back(name);
            }
        }
    }
    catch (const std::exception &e)
    {
        logMessage(std::string("[DIR READ FAILED] ") + e.what());
        return {};
    }

    std::sort(temp_list.begin(), temp_list.end());

    app.cache.dir_cache[dir_path] = temp_list;

    return temp_list;
}
