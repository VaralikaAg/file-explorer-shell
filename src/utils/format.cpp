#include "myheader.h"

std::string humanReadableSize(off_t size) {
    if (size == 0) return "0 B";
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double sizeInUnit = (double)size;

    while (sizeInUnit >= 1024 && unitIndex < 5) {
        sizeInUnit /= 1024;
        unitIndex++;
    }

    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << size << " B";
    } else {
        oss << std::fixed << std::setprecision(1) << sizeInUnit << " " << units[unitIndex];
    }
    return oss.str();
}

std::string truncateStr(const std::string &str, size_t maxLength) {
    if (str.length() <= maxLength) {
        return str;
    }
    if (maxLength <= 3) return "...";
    return str.substr(0, maxLength - 3) + "...";
}