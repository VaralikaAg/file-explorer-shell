#include "myheader.h"

int InvertedIndex::getWordId(const std::string &word)
{
    auto it = wordToId.find(word);
    if (it != wordToId.end())
        return it->second;

    int id = idToWord.size();
    wordToId[word] = id;
    idToWord.push_back(word);
    return id;
}

void InvertedIndex::indexPath(const std::string &path)
{
    int file_id;

    /* ---------- FILE ID ---------- */
    auto fit = fileToId.find(path);
    if (fit == fileToId.end()) {

        if (!app.indexing.freeFileIds.empty()) {
            // reuse deleted file id
            file_id = app.indexing.freeFileIds.back();
            app.indexing.freeFileIds.pop_back();
            idToFile[file_id] = path;
        } else {
            // create new file id
            file_id = idToFile.size();
            idToFile.push_back(path);
        }

        fileToId[path] = file_id;
    } else {
        file_id = fit->second;
    }

    /* ---------- INDEX FILE / DIR NAME ---------- */
    size_t pos = path.find_last_of("/\\");
    std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);
    std::string cleanName = normalizeWord(name);

    if (!cleanName.empty()) {
        int wid = getWordId(cleanName);
        invertedIndex[wid].insert(file_id);
        forwardIndex[file_id].insert(wid);
    }

    /* ---------- STOP IF DIRECTORY ---------- */
    if (isDirectory(path)) return;
    if (!isRegularFile(path)) return;

    std::ifstream fp(path);
    if (!fp) return;

    /* ---------- INDEX FILE CONTENT ---------- */
    std::string line, word;
    while (getline(fp, line)) {
        std::stringstream ss(line);
        while (ss >> word) {
            std::string clean = normalizeWord(word);
            if (clean.empty()) continue;

            int word_id = getWordId(clean);

            invertedIndex[word_id].insert(file_id);   // backward
            forwardIndex[file_id].insert(word_id);    // forward
        }
    }

    fp.close();
}

void InvertedIndex::indexAllOnce(std::queue<std::string> &paths)
{
    while (!paths.empty()) {
        std::string path = paths.front();
        paths.pop();
        indexPath(path);
    }
}

void InvertedIndex::search(const std::string &query)
{
    app.search.foundPaths.clear();   // GLOBAL vector<string>

    // auto t1 = chrono::high_resolution_clock::now();

    std::stringstream ss(query);
    std::string token;
    std::vector<int> queryWordIds;

    /* 1️⃣ Normalize + map to word IDs */
    while (ss >> token) {
        std::string clean = normalizeWord(token);
        if (clean.empty()) continue;

        auto it = wordToId.find(clean);
        if (it == wordToId.end())
            return;   // AND semantics → fail early

        queryWordIds.push_back(it->second);
    }

    if (queryWordIds.empty()) return;

    /* 2️⃣ Count files */
    std::unordered_map<int, int> fileCounter;

    for (int wid : queryWordIds) {
        auto it = invertedIndex.find(wid);
        if (it == invertedIndex.end())
            return;

        for (int fid : it->second) {
            fileCounter[fid]++;
        }
    }

    /* 3️⃣ Intersection */
    int required = queryWordIds.size();
    for (auto &p : fileCounter) {
        if (p.second == required) {
            if (!idToFile[p.first].empty())
                app.search.foundPaths.push_back(idToFile[p.first]);
        }
    }

}

void InvertedIndex::rectifyIndex(
    RectifyAction action,
    const std::vector<std::string> &oldPaths,
    const std::vector<std::string> &newPaths)
{
    switch (action) {

    case RectifyAction::CREATE:
    case RectifyAction::COPY:
        // index all new paths
        for (const auto &p : newPaths) {
            indexPath(p);
        }
        break;

    case RectifyAction::RENAME:
        // 1-to-1 mapping: oldPaths[i] → newPaths[i]
        for (size_t i = 0; i < oldPaths.size(); i++) {
            removePath(oldPaths[i]);
            indexPath(newPaths[i]);
        }
        break;

    case RectifyAction::DELETE:
        // remove all old paths
        for (const auto &p : oldPaths) {
            removePath(p);
        }
        break;
    }
}

void InvertedIndex::removePath(const std::string &path)
{
    auto fit = fileToId.find(path);
    if (fit == fileToId.end())
        return;

    int file_id = fit->second;

    /* 1️⃣ Remove from forward index */
    auto fwit = forwardIndex.find(file_id);
    if (fwit != forwardIndex.end()) {
        for (int word_id : fwit->second) {
            auto iit = invertedIndex.find(word_id);
            if (iit != invertedIndex.end()) {
                iit->second.erase(file_id);

                // cleanup empty postings
                if (iit->second.empty()) {
                    invertedIndex.erase(word_id);
                }
            }
        }
        forwardIndex.erase(file_id);
    }

    /* 2️⃣ Remove file mappings */
    fileToId.erase(path);
    idToFile[file_id] = "";
    app.indexing.freeFileIds.push_back(file_id);
}