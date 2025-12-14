#include "myheader.h"

bool isRegularFile(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

bool isDirectoryPath(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

int InvertedIndex::getWordId(const string &word)
{
    auto it = wordToId.find(word);
    if (it != wordToId.end())
        return it->second;

    int id = idToWord.size();
    wordToId[word] = id;
    idToWord.push_back(word);
    return id;
}

void InvertedIndex::indexPath(const string &path)
{
    int file_id;

    /* ---------- FILE ID ---------- */
    auto fit = fileToId.find(path);
    if (fit == fileToId.end()) {
        file_id = idToFile.size();
        fileToId[path] = file_id;
        idToFile.push_back(path);
    } else {
        file_id = fit->second;
    }

    /* ---------- INDEX FILE / DIR NAME ---------- */
    size_t pos = path.find_last_of("/\\");
    string name = (pos == string::npos) ? path : path.substr(pos + 1);
    string cleanName = normalizeWord(name);

    if (!cleanName.empty()) {
        int wid = getWordId(cleanName);
        invertedIndex[wid].insert(file_id);
        forwardIndex[file_id].insert(wid);
    }

    /* ---------- STOP IF DIRECTORY ---------- */
    if (isDirectoryPath(path)) return;
    if (!isRegularFile(path)) return;

    ifstream fp(path);
    if (!fp) return;

    /* ---------- INDEX FILE CONTENT ---------- */
    string line, word;
    while (getline(fp, line)) {
        stringstream ss(line);
        while (ss >> word) {
            string clean = normalizeWord(word);
            if (clean.empty()) continue;

            int word_id = getWordId(clean);

            invertedIndex[word_id].insert(file_id);   // backward
            forwardIndex[file_id].insert(word_id);    // forward
        }
    }

    fp.close();
}



void InvertedIndex::indexAllOnce(queue<string> &paths)
{
    while (!paths.empty()) {
        string path = paths.front();
        paths.pop();
        indexPath(path);
    }
}

void InvertedIndex::search(const string &query)
{
    auto t1 = chrono::high_resolution_clock::now();

    stringstream ss(query);
    string token;

    vector<int> queryWordIds;

    /* 1️⃣ Normalize words & map to word IDs */
    while (ss >> token) {
        string clean = normalizeWord(token);
        if (clean.empty())
            continue;

        auto wit = wordToId.find(clean);
        if (wit == wordToId.end()) {
            logMessage("Word not found: " + clean);
            return; // AND query → fail early
        }

        queryWordIds.push_back(wit->second);
    }

    if (queryWordIds.empty()) {
        logMessage("Empty or invalid query");
        return;
    }

    /* 2️⃣ Count file occurrences */
    unordered_map<int, int> fileCounter;

    for (int word_id : queryWordIds) {
        auto it = invertedIndex.find(word_id);
        if (it == invertedIndex.end())
            return;

        for (int file_id : it->second) {
            fileCounter[file_id]++;
        }
    }

    /* 3️⃣ Filter files present in ALL words */
    vector<int> results;
    int requiredCount = queryWordIds.size();

    for (auto &p : fileCounter) {
        if (p.second == requiredCount) {
            results.push_back(p.first);
        }
    }

    auto t2 = chrono::high_resolution_clock::now();

    logMessage("Search took: " +
        to_string(chrono::duration_cast<chrono::milliseconds>(t2 - t1).count()) +
        " ms");

    /* 4️⃣ Output */
    if (results.empty()) {
        logMessage("No files matched all query words");
        return;
    }

    logMessage("Matched files:");
    for (int file_id : results) {
        logMessage(idToFile[file_id]);
    }
}


