#include "myheader.hpp"

void logMessage(const std::string &message)
{
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);

    // Ensure directory exists
    std::filesystem::create_directories("logs");

    static std::ofstream log_file("logs/debug.log", std::ios::app);

    if (!log_file.is_open())
    {
        std::cerr << "Error opening log file!" << std::endl;
        return;
    }

    log_file << message << '\n';
    log_file.flush();
}