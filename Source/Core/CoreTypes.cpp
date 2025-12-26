#include "CoreTypes.h"
#include <fstream>

static std::ofstream logFile;

void FLog::Log(ELogLevel Level, const std::string& Message)
{
    // Open log file on first use
    if (!logFile.is_open())
    {
        logFile.open("UE5MinimalRenderer.log", std::ios::out | std::ios::trunc);
    }
    
    const char* Prefix = "";
    switch (Level)
    {
        case ELogLevel::Info:    Prefix = "[INFO] "; break;
        case ELogLevel::Warning: Prefix = "[WARNING] "; break;
        case ELogLevel::Error:   Prefix = "[ERROR] "; break;
    }
    
    std::string fullMessage = std::string(Prefix) + Message;
    
    // Write to file only
    if (logFile.is_open())
    {
        logFile << fullMessage << std::endl;
        logFile.flush();  // Ensure it's written immediately
    }
}
