#include "myheader.h"

void invalidateDirCache(const string &dirPath)
{
    auto it = app.cache.dirCache.find(dirPath);
    if (it != app.cache.dirCache.end())
        app.cache.dirCache.erase(it);
}

int getDirectoryCount(const fs::path &path)
{
    std::string dirPath = path.string();

    auto it = app.cache.dirCache.find(dirPath);
    if (it != app.cache.dirCache.end())
    {
        app.nav.fileList = it->second;
        return app.nav.fileList.size();
    }

    std::vector<std::string> tempList;

    try
    {
        for (const auto &entry : fs::directory_iterator(path))
        {
            std::string name = entry.path().filename().string();
            if (name == "." || name == "..") continue;
            tempList.push_back(name);
        }
    }
    catch (const std::exception &e)
    {
        logMessage(std::string("[DIR READ FAILED] ") + e.what());
        return 0;
    }

    std::sort(tempList.begin(), tempList.end());

    app.cache.dirCache[dirPath] = tempList;
    app.nav.fileList = tempList;

    return app.nav.fileList.size();
}