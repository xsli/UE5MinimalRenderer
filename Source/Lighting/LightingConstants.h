#pragma once

#include "../Core/CoreTypes.h"
#include "Light.h"
#include <DirectXMath.h>

/**
 * FLightingConstants - GPU constant buffer data for lighting calculations
 * Must match the HLSL LightingBuffer structure exactly
 * Aligned to 16-byte boundaries for GPU compatibility
 */
struct FLightingConstants
{
    // Model matrix for transforming normals to world space
    DirectX::XMMATRIX ModelMatrix;           // 64 bytes
    
    // Camera position for specular calculations
    DirectX::XMFLOAT4 CameraPosition;        // 16 bytes (xyz = pos, w = unused)
    
    // Scene ambient light
    DirectX::XMFLOAT4 AmbientLight;          // 16 bytes (xyz = color, w = intensity)
    
    // Directional light
    DirectX::XMFLOAT4 DirLightDirection;     // 16 bytes (xyz = dir, w = enabled)
    DirectX::XMFLOAT4 DirLightColor;         // 16 bytes (xyz = color, w = intensity)
    
    // Point lights (up to 4)
    DirectX::XMFLOAT4 PointLight0Position;   // 16 bytes
    DirectX::XMFLOAT4 PointLight0Color;      // 16 bytes
    DirectX::XMFLOAT4 PointLight0Params;     // 16 bytes (x = radius, y = falloff)
    
    DirectX::XMFLOAT4 PointLight1Position;
    DirectX::XMFLOAT4 PointLight1Color;
    DirectX::XMFLOAT4 PointLight1Params;
    
    DirectX::XMFLOAT4 PointLight2Position;
    DirectX::XMFLOAT4 PointLight2Color;
    DirectX::XMFLOAT4 PointLight2Params;
    
    DirectX::XMFLOAT4 PointLight3Position;
    DirectX::XMFLOAT4 PointLight3Color;
    DirectX::XMFLOAT4 PointLight3Params;
    
    // Material properties
    DirectX::XMFLOAT4 MaterialDiffuse;       // 16 bytes (xyz = color, w = unused)
    DirectX::XMFLOAT4 MaterialSpecular;      // 16 bytes (xyz = color, w = shininess)
    DirectX::XMFLOAT4 MaterialAmbient;       // 16 bytes (xyz = color, w = unused)
    
    // Total: 64 + 16*20 = 384 bytes
    // Note: DX12 constant buffers are automatically aligned to 256 bytes by CreateConstantBuffer()
    
    FLightingConstants()
    {
        Clear();
    }
    
    void Clear()
    {
        ModelMatrix = DirectX::XMMatrixIdentity();
        CameraPosition = { 0.0f, 0.0f, 0.0f, 0.0f };
        AmbientLight = { 0.1f, 0.1f, 0.15f, 1.0f };
        
        DirLightDirection = { 0.0f, -1.0f, 0.0f, 0.0f };
        DirLightColor = { 1.0f, 1.0f, 1.0f, 0.0f };
        
        ClearPointLight(0);
        ClearPointLight(1);
        ClearPointLight(2);
        ClearPointLight(3);
        
        SetDefaultMaterial();
    }
    
    void ClearPointLight(int index)
    {
        switch (index)
        {
            case 0:
                PointLight0Position = { 0.0f, 0.0f, 0.0f, 0.0f };
                PointLight0Color = { 1.0f, 1.0f, 1.0f, 0.0f };
                PointLight0Params = { 10.0f, 2.0f, 0.0f, 0.0f };
                break;
            case 1:
                PointLight1Position = { 0.0f, 0.0f, 0.0f, 0.0f };
                PointLight1Color = { 1.0f, 1.0f, 1.0f, 0.0f };
                PointLight1Params = { 10.0f, 2.0f, 0.0f, 0.0f };
                break;
            case 2:
                PointLight2Position = { 0.0f, 0.0f, 0.0f, 0.0f };
                PointLight2Color = { 1.0f, 1.0f, 1.0f, 0.0f };
                PointLight2Params = { 10.0f, 2.0f, 0.0f, 0.0f };
                break;
            case 3:
                PointLight3Position = { 0.0f, 0.0f, 0.0f, 0.0f };
                PointLight3Color = { 1.0f, 1.0f, 1.0f, 0.0f };
                PointLight3Params = { 10.0f, 2.0f, 0.0f, 0.0f };
                break;
        }
    }
    
    void SetDefaultMaterial()
    {
        MaterialDiffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
        MaterialSpecular = { 1.0f, 1.0f, 1.0f, 32.0f };
        MaterialAmbient = { 0.1f, 0.1f, 0.1f, 1.0f };
    }
    
    void SetModelMatrix(const FMatrix4x4& Matrix)
    {
        // DirectXMath uses row-major storage, HLSL uses column-major by default.
        // Transpose is required to convert from CPU row-major to GPU column-major.
        ModelMatrix = DirectX::XMMatrixTranspose(Matrix.Matrix);
    }
    
    void SetCameraPosition(const FVector& Pos)
    {
        CameraPosition = { Pos.X, Pos.Y, Pos.Z, 0.0f };
    }
    
    void SetAmbientLight(const FColor& Color, float Intensity = 1.0f)
    {
        AmbientLight = { Color.R, Color.G, Color.B, Intensity };
    }
    
    void SetDirectionalLight(const FDirectionalLight* Light)
    {
        if (Light && Light->IsEnabled())
        {
            const FVector& dir = Light->GetDirection();
            const FColor& col = Light->GetColor();
            DirLightDirection = { dir.X, dir.Y, dir.Z, 1.0f };
            DirLightColor = { col.R, col.G, col.B, Light->GetIntensity() };
        }
        else
        {
            DirLightDirection = { 0.0f, -1.0f, 0.0f, 0.0f };
            DirLightColor = { 1.0f, 1.0f, 1.0f, 0.0f };
        }
    }
    
    void SetPointLight(int Index, const FPointLight* Light)
    {
        DirectX::XMFLOAT4 pos, color, params;
        
        if (Light && Light->IsEnabled())
        {
            const FVector& p = Light->GetPosition();
            const FColor& c = Light->GetColor();
            pos = { p.X, p.Y, p.Z, 1.0f };
            color = { c.R, c.G, c.B, Light->GetIntensity() };
            params = { Light->GetRadius(), Light->GetFalloffExponent(), 0.0f, 0.0f };
        }
        else
        {
            pos = { 0.0f, 0.0f, 0.0f, 0.0f };
            color = { 1.0f, 1.0f, 1.0f, 0.0f };
            params = { 10.0f, 2.0f, 0.0f, 0.0f };
        }
        
        switch (Index)
        {
            case 0:
                PointLight0Position = pos;
                PointLight0Color = color;
                PointLight0Params = params;
                break;
            case 1:
                PointLight1Position = pos;
                PointLight1Color = color;
                PointLight1Params = params;
                break;
            case 2:
                PointLight2Position = pos;
                PointLight2Color = color;
                PointLight2Params = params;
                break;
            case 3:
                PointLight3Position = pos;
                PointLight3Color = color;
                PointLight3Params = params;
                break;
        }
    }
    
    void SetMaterial(const FMaterial& Mat)
    {
        MaterialDiffuse = { Mat.DiffuseColor.R, Mat.DiffuseColor.G, Mat.DiffuseColor.B, 1.0f };
        MaterialSpecular = { Mat.SpecularColor.R, Mat.SpecularColor.G, Mat.SpecularColor.B, Mat.Shininess };
        MaterialAmbient = { Mat.AmbientColor.R, Mat.AmbientColor.G, Mat.AmbientColor.B, 1.0f };
    }
};
