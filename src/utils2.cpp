#include "myheader.h"

void logMessage(const std::string& message) {
    std::ofstream logFile("logs/debug.log", std::ios_base::app); // Open log file in append mode
    if (logFile.is_open()) {
        logFile << message << std::endl; // Write message to file
    } else {
        std::cerr << "Error opening log file!" << std::endl;
    }
}

static const unordered_set<string> STOPWORDS = {
    "a","an","the","and","or","but","if","while","is","am","are","was","were",
    "to","of","in","on","for","with","as","by","at","from","this","that",
    "it","be","been","being","have","has","had","do","does","did"
};

string normalizeWord(const string &input)
{
    string out;
    out.reserve(input.size());

    // lowercase + strip punctuation
    for (char c : input) {
        if (isalnum(static_cast<unsigned char>(c))) {
            out += tolower(static_cast<unsigned char>(c));
        }
    }

    // remove empty
    if (out.empty())
        return "";

    // remove stopwords
    if (STOPWORDS.count(out))
        return "";

    return out;
}
