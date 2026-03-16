#include "myheader.h"

void searchCommand(bool check_dir, bool check_file, std::string filename)
{
    if(filename.empty()) return;

    std::string path = app.nav.currPath;

    app.search.foundPaths.resize(0);
    transform(filename.begin(), filename.end(), filename.begin(), ::tolower); 

    auto start = std::chrono::high_resolution_clock::now();
    searchAnything(path.c_str(), filename, check_file, check_dir);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    logMessage("Search took: " + std::to_string(elapsed.count()) + " ms");
    clearScreen();
    displaySearchResults();
}

void copy() {
    std::vector<std::string> tempClipboard;

    if (!app.selection.selectedFiles.empty()) {
        for (auto &path : app.selection.selectedFiles) {
            if (path.empty())
                throw std::runtime_error("Empty path in selectedFiles");

            tempClipboard.push_back(path);
        }
    } else {
        int index = app.nav.xcurr + app.nav.up_screen - 1;

        if (index < 0 || index >= (int)app.nav.fileList.size())
            throw std::out_of_range("Invalid cursor index");

        std::string file = app.nav.fileList[index];
        tempClipboard.push_back(app.nav.currPath + "/" + file);
    }

    app.selection.clipboard = std::move(tempClipboard);
}

std::string paste()
{
    if (app.selection.clipboard.empty())
    return "";
    
    std::vector<std::string> pastedFiles;
    std::string fileName="";
    for (auto &src : app.selection.clipboard) {
        fs::path sourcePath(src);
        fs::path dest = fs::path(app.nav.currPath) / sourcePath.filename();

        if (fs::is_directory(sourcePath)) {
            fs::copy(sourcePath, dest,
                fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        } else {
            fs::copy_file(sourcePath, dest,
                fs::copy_options::overwrite_existing);
        }

        pastedFiles.push_back(dest.string());
        if(fileName.length()==0) fileName=dest.filename().string();
    }

    if (app.config.indexingEnabled) {
        app.indexing.index.rectifyIndex(
            RectifyAction::COPY, {}, pastedFiles
        );
    }

    app.selection.selectedFiles.clear();
    return fileName;
}

void deleteSelectedItems()
{
    std::vector<std::string> targets;

    if (!app.selection.selectedFiles.empty()) {
        for (auto &p : app.selection.selectedFiles)
            targets.push_back(p);
    } else {
        std::string file = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
        targets.push_back(app.nav.currPath + "/" + file);
    }

    for (auto &path : targets) {
        fs::remove_all(path);
    }

    if (app.config.indexingEnabled) {
        app.indexing.index.rectifyIndex(
            RectifyAction::DELETE, targets, {}
        );
    }

    app.selection.selectedFiles.clear();
}

bool renameItem(const std::string &selectedFile, const std::string &newName)
{
    if (newName.empty()) return false;

    std::string oldPath = app.nav.currPath + "/" + selectedFile;
    std::string newPath = app.nav.currPath + "/" + newName;

    if (rename(oldPath.c_str(), newPath.c_str()) != 0)
        return false;

    if (app.config.indexingEnabled) {
        app.indexing.index.rectifyIndex(
            RectifyAction::RENAME, {oldPath}, {newPath}
        );
    }

    return true;
}

bool createFile(const std::string &fileName)
{
    if (fileName.empty()) return false;

    std::string filePath = app.nav.currPath + "/" + fileName;

    std::ofstream file(filePath);
    if (!file) return false;
    file.close();

    if (app.config.indexingEnabled) {
        app.indexing.index.rectifyIndex(
            RectifyAction::CREATE, {}, {filePath}
        );
    }

    return true;
}

bool createDirectory(const std::string &dirName)
{
    if (dirName.empty()) return false;

    std::string dirPath = app.nav.currPath + "/" + dirName;

    if (mkdir(dirPath.c_str(), 0777) != 0)
        return false;

    if (app.config.indexingEnabled) {
        app.indexing.index.rectifyIndex(
            RectifyAction::CREATE, {}, {dirPath}
        );
    }

    return true;
}