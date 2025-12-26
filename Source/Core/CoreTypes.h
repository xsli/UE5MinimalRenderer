#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <DirectXMath.h>

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

// Matrix type - wrapper around DirectXMath
struct FMatrix4x4 {
    DirectX::XMMATRIX Matrix;
    
    FMatrix4x4() : Matrix(DirectX::XMMatrixIdentity()) {}
    FMatrix4x4(const DirectX::XMMATRIX& InMatrix) : Matrix(InMatrix) {}
    
    // Static factory methods
    static FMatrix4x4 Identity()
{
        return FMatrix4x4(DirectX::XMMatrixIdentity());
    }
    
    static FMatrix4x4 Translation(float X, float Y, float Z)
{
        return FMatrix4x4(DirectX::XMMatrixTranslation(X, Y, Z));
    }
    
    static FMatrix4x4 RotationX(float Angle)
{
        return FMatrix4x4(DirectX::XMMatrixRotationX(Angle));
    }
    
    static FMatrix4x4 RotationY(float Angle)
{
        return FMatrix4x4(DirectX::XMMatrixRotationY(Angle));
    }
    
    static FMatrix4x4 RotationZ(float Angle)
{
        return FMatrix4x4(DirectX::XMMatrixRotationZ(Angle));
    }
    
    static FMatrix4x4 Scaling(float X, float Y, float Z)
{
        return FMatrix4x4(DirectX::XMMatrixScaling(X, Y, Z));
    }
    
    // Perspective projection matrix (left-handed, DirectX style)
    static FMatrix4x4 PerspectiveFovLH(float FovY, float AspectRatio, float NearZ, float FarZ)
{
        return FMatrix4x4(DirectX::XMMatrixPerspectiveFovLH(FovY, AspectRatio, NearZ, FarZ));
    }
    
    // Look-at view matrix (left-handed, DirectX style)
    static FMatrix4x4 LookAtLH(const FVector& EyePosition, const FVector& FocusPosition, const FVector& UpDirection)
{
        DirectX::XMVECTOR eye = DirectX::XMVectorSet(EyePosition.X, EyePosition.Y, EyePosition.Z, 0.0f);
        DirectX::XMVECTOR focus = DirectX::XMVectorSet(FocusPosition.X, FocusPosition.Y, FocusPosition.Z, 0.0f);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(UpDirection.X, UpDirection.Y, UpDirection.Z, 0.0f);
        return FMatrix4x4(DirectX::XMMatrixLookAtLH(eye, focus, up));
    }
    
    // Matrix multiplication
    FMatrix4x4 operator*(const FMatrix4x4& Other) const
{
        return FMatrix4x4(DirectX::XMMatrixMultiply(Matrix, Other.Matrix));
    }
    
    // Transpose
    FMatrix4x4 Transpose() const
{
        return FMatrix4x4(DirectX::XMMatrixTranspose(Matrix));
    }
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
