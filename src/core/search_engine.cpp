#include "myheader.h"

void searchAnything(const fs::path &dir, const std::string &filename, bool check_file, bool check_dir)
{
    for (const auto &entry : fs::directory_iterator(dir))
    {
        fs::path fullPath = entry.path();

        std::string entryName = fullPath.filename().string();
        std::transform(entryName.begin(), entryName.end(),
                       entryName.begin(), ::tolower);

        bool isDir = isDirectory(fullPath);

        // 🔹 Match logic (no duplication)
        if ((isDir && check_dir) || (!isDir && check_file))
        {
            if (entryName.find(filename) != std::string::npos)
            {
                app.search.foundPaths.push_back(fullPath.string());
            }
        }

        // 🔹 Recurse
        if (isDir)
        {
            searchAnything(fullPath, filename, check_file, check_dir);
        }
    }
}
