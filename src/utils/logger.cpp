#include "myheader.h"

void logMessage(const std::string &message)
{
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);

    // Ensure directory exists
    std::filesystem::create_directories("logs");

    static std::ofstream logFile("logs/debug.log", std::ios::app);

    if (!logFile.is_open())
    {
        std::cerr << "Error opening log file!" << std::endl;
        return;
    }

    logFile << message << '\n';
    logFile.flush();
}