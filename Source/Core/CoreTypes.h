#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// Basic types
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

// Math types
struct FVector2D {
    float X, Y;
    FVector2D(float x = 0.0f, float y = 0.0f) : X(x), Y(y) {}
};

struct FVector {
    float X, Y, Z;
    FVector(float x = 0.0f, float y = 0.0f, float z = 0.0f) : X(x), Y(y), Z(z) {}
};

struct FVector4 {
    float X, Y, Z, W;
    FVector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) : X(x), Y(y), Z(z), W(w) {}
};

struct FColor {
    float R, G, B, A;
    FColor(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) : R(r), G(g), B(b), A(a) {}
};

// Logging
enum class ELogLevel {
    Info,
    Warning,
    Error
};

class FLog {
public:
    static void Log(ELogLevel Level, const std::string& Message);
};
