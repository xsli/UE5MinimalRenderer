#include "Light.h"
#include <algorithm>
#include <cmath>

// FLight implementation
FLight::FLight()
    : Color(1.0f, 1.0f, 1.0f, 1.0f)
    , Intensity(1.0f)
    , bIsEnabled(true)
    , Position(0.0f, 0.0f, 0.0f)
{
}

// FDirectionalLight implementation
FDirectionalLight::FDirectionalLight()
    : Direction(0.0f, -1.0f, 0.0f)  // Default: light coming from above
{
}

void FDirectionalLight::SetDirection(const FVector& InDirection)
{
    // Normalize the direction
    float length = std::sqrt(InDirection.X * InDirection.X + 
                            InDirection.Y * InDirection.Y + 
                            InDirection.Z * InDirection.Z);
    if (length > 0.0001f)
    {
        Direction = FVector(InDirection.X / length, 
                           InDirection.Y / length, 
                           InDirection.Z / length);
    }
    else
    {
        Direction = FVector(0.0f, -1.0f, 0.0f);  // Default down
    }
}

// FPointLight implementation
FPointLight::FPointLight()
    : Radius(10.0f)
    , FalloffExponent(2.0f)  // Quadratic falloff (physically accurate)
{
}

float FPointLight::GetAttenuation(float Distance) const
{
    if (Distance >= Radius)
    {
        return 0.0f;
    }
    
    // Smooth inverse square law with radius cutoff
    // UE4/5 uses a similar approach for point light attenuation
    float normalizedDist = Distance / Radius;
    float attenuation = 1.0f / (1.0f + std::pow(normalizedDist, FalloffExponent));
    
    // Smooth falloff at radius boundary
    float falloff = 1.0f - std::pow(normalizedDist, 4.0f);
    falloff = std::max(0.0f, falloff);
    
    return attenuation * falloff;
}

// FLightScene implementation
FLightScene::FLightScene()
    : AmbientLight(0.1f, 0.1f, 0.15f, 1.0f)  // Slight blue ambient for outdoor
{
}

FLightScene::~FLightScene()
{
    ClearLights();
}

void FLightScene::AddLight(FLight* Light)
{
    if (Light)
    {
        Lights.push_back(Light);
        FLog::Log(ELogLevel::Info, std::string("FLightScene::AddLight - Total lights: ") + std::to_string(Lights.size()));
    }
}

void FLightScene::RemoveLight(FLight* Light)
{
    auto it = std::find(Lights.begin(), Lights.end(), Light);
    if (it != Lights.end())
    {
        Lights.erase(it);
        FLog::Log(ELogLevel::Info, std::string("FLightScene::RemoveLight - Remaining lights: ") + std::to_string(Lights.size()));
    }
}

void FLightScene::ClearLights()
{
    for (FLight* Light : Lights)
    {
        delete Light;
    }
    Lights.clear();
    FLog::Log(ELogLevel::Info, "FLightScene::ClearLights - All lights cleared");
}

std::vector<FDirectionalLight*> FLightScene::GetDirectionalLights() const
{
    std::vector<FDirectionalLight*> result;
    for (FLight* light : Lights)
    {
        if (light->IsEnabled() && light->GetType() == ELightType::Directional)
        {
            result.push_back(static_cast<FDirectionalLight*>(light));
        }
    }
    return result;
}

std::vector<FPointLight*> FLightScene::GetPointLights() const
{
    std::vector<FPointLight*> result;
    for (FLight* light : Lights)
    {
        if (light->IsEnabled() && light->GetType() == ELightType::Point)
        {
            result.push_back(static_cast<FPointLight*>(light));
        }
    }
    return result;
}
