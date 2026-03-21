#include "myheader.h"

static const std::unordered_set<std::string> STOPWORDS = {
    // Basic Pronouns & Articles
    "a", "an", "the", "and", "or", "but", "if", "while", "is", "am", "are", "was", "were",
    "be", "been", "being", "have", "has", "had", "do", "does", "did", "it", "its", "it's",
    "i", "me", "my", "myself", "we", "us", "our", "ours", "ourselves", "you", "your", "yours",
    "yourself", "yourselves", "he", "him", "his", "himself", "she", "her", "hers", "herself",
    "they", "them", "their", "theirs", "themselves", "what", "which", "who", "whom", "this",
    "that", "these", "those", "which", "whose", "whom",

    // Prepositions & Conjunctions
    "to", "of", "in", "on", "for", "with", "as", "by", "at", "from", "into", "through",
    "during", "before", "after", "above", "below", "up", "down", "out", "over", "under",
    "again", "further", "then", "once", "here", "there", "when", "where", "why", "how",
    "all", "any", "both", "each", "few", "more", "most", "other", "some", "such", "no", 
    "nor", "not", "only", "own", "same", "so", "than", "too", "very", "can", "will", "just",
    "should", "now", "between", "against", "because", "until",

    // Common Verbs & Noise
    "go", "get", "make", "take", "say", "come", "see", "know", "think", "look", "want",
    "give", "use", "find", "tell", "ask", "work", "seem", "feel", "try", "leave", "call",
    "must", "might", "could", "would", "shall", "ought", "need", "used", "become",

    // Numerics & Miscellaneous
    "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
    "first", "second", "third", "total", "count", "size", "show", "get", "set"
};

std::string normalizeWord(const std::string &input)
{
    if (input.empty()) return "";

    std::string out;
    out.reserve(input.size());

    // lowercase + allow alphanumeric and specific special chars (@#_-$&)
    for (char c : input)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc))
        {
            out += static_cast<char>(std::tolower(uc));
        }
        else if (uc == '@' || uc == '#' || uc == '_' || uc == '-' || uc == '$' || uc == '&')
        {
            out += static_cast<char>(uc);
        }
    }

    // remove empty or stopwords
    if (out.empty() || STOPWORDS.count(out))
        return "";

    return out;
}