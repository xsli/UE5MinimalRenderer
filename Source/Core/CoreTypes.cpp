#include "CoreTypes.h"
#include <iostream>

void FLog::Log(ELogLevel Level, const std::string& Message) {
    const char* Prefix = "";
    switch (Level) {
        case ELogLevel::Info:    Prefix = "[INFO] "; break;
        case ELogLevel::Warning: Prefix = "[WARNING] "; break;
        case ELogLevel::Error:   Prefix = "[ERROR] "; break;
    }
    std::cout << Prefix << Message << std::endl;
}
