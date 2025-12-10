#include "myheader.h"

void logMessage(const std::string& message) {
    std::ofstream logFile("logs/debug.log", std::ios_base::app); // Open log file in append mode
    if (logFile.is_open()) {
        logFile << message << std::endl; // Write message to file
    } else {
        std::cerr << "Error opening log file!" << std::endl;
    }
}