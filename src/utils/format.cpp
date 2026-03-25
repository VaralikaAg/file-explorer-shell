#include "myheader.hpp"

std::string humanReadableSize(off_t size) {
    if (size == 0) return "0 B";
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size_in_unit = (double)size;

    while (size_in_unit >= 1024 && unit_index < 5) {
        size_in_unit /= 1024;
        unit_index++;
    }

    std::ostringstream oss;
    if (unit_index == 0) {
        oss << size << " B";
    } else {
        oss << std::fixed << std::setprecision(1) << size_in_unit << " " << units[unit_index];
    }
    return oss.str();
}

std::string truncateStr(const std::string &str, size_t max_length) {
    std::string result = str;
    if (str.length() > max_length) {
        if (max_length <= 3) result = "...";
        else result = str.substr(0, max_length - 3) + "...";
    }
    
    if (result.length() < max_length) {
        result.append(max_length - result.length(), ' ');
    }
    return result;
}