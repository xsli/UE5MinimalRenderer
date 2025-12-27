#pragma once

#include "../Core/CoreTypes.h"
#include <vector>

/**
 * Light types enumeration - maps to UE5 light component types
 * Designed for future expansion to include spot lights, area lights, etc.
 */
enum class ELightType
{
    Directional,
    Point,
    // Future types:
    // Spot,
    // RectArea,
    // Sky,
};

/**
 * FLight - Base class for all light types
 * Follows UE5's component-based light system design
 * Ready for future deferred rendering integration
 */
class FLight
{
public:
    FLight();
    virtual ~FLight() = default;

    // Light type identification
    virtual ELightType GetType() const = 0;

    // Common light properties
    void SetColor(const FColor& InColor) { Color = InColor; }
    const FColor& GetColor() const { return Color; }

    void SetIntensity(float InIntensity) { Intensity = InIntensity; }
    float GetIntensity() const { return Intensity; }

    void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }
    bool IsEnabled() const { return bIsEnabled; }

    // Get the final light color (color * intensity)
    FColor GetLightColor() const
    {
        return FColor(Color.R * Intensity, Color.G * Intensity, Color.B * Intensity, Color.A);
    }

    // Transform accessors for light position/direction
    void SetPosition(const FVector& InPosition) { Position = InPosition; }
    const FVector& GetPosition() const { return Position; }

protected:
    FColor Color;
    float Intensity;
    bool bIsEnabled;
    FVector Position;
};

/**
 * FDirectionalLight - Parallel light source (like the sun)
 * Direction is the direction light rays travel (from light toward scene)
 * Used in UE5 for outdoor environments, skylighting
 */
class FDirectionalLight : public FLight
{
public:
    FDirectionalLight();
    virtual ~FDirectionalLight() override = default;

    virtual ELightType GetType() const override { return ELightType::Directional; }

    // Direction the light is pointing (normalized)
    void SetDirection(const FVector& InDirection);
    const FVector& GetDirection() const { return Direction; }

    // Get light direction vector (normalized, points toward light source for shader calculations)
    FVector GetLightDirection() const
    {
        // Return inverted direction for shader (points toward light)
        return FVector(-Direction.X, -Direction.Y, -Direction.Z);
    }

private:
    FVector Direction;  // Direction light rays travel
};

/**
 * FPointLight - Omni-directional light source
 * Emits light in all directions from a single point
 * Used in UE5 for local illumination (lamps, torches, etc.)
 */
class FPointLight : public FLight
{
public:
    FPointLight();
    virtual ~FPointLight() override = default;

    virtual ELightType GetType() const override { return ELightType::Point; }

    // Attenuation control
    void SetRadius(float InRadius) { Radius = InRadius; }
    float GetRadius() const { return Radius; }

    // Attenuation falloff (1.0 = linear, 2.0 = quadratic)
    void SetFalloffExponent(float InExponent) { FalloffExponent = InExponent; }
    float GetFalloffExponent() const { return FalloffExponent; }

    // Calculate attenuation at a given distance
    float GetAttenuation(float Distance) const;

private:
    float Radius;           // Light influence radius
    float FalloffExponent;  // Falloff curve exponent
};

/**
 * FMaterial - Surface material properties for Phong/Blinn-Phong shading
 * Designed with future PBR extension in mind:
 *  - Diffuse -> BaseColor
 *  - Specular + Shininess -> Roughness/Metallic
 */
struct FMaterial
{
    FColor DiffuseColor;    // Base surface color
    FColor SpecularColor;   // Specular highlight color
    FColor AmbientColor;    // Ambient contribution
    float Shininess;        // Specular power (1-128+)
    
    // Emission for self-illuminating surfaces
    FColor EmissiveColor;
    
    FMaterial()
        : DiffuseColor(0.8f, 0.8f, 0.8f, 1.0f)
        , SpecularColor(1.0f, 1.0f, 1.0f, 1.0f)
        , AmbientColor(0.1f, 0.1f, 0.1f, 1.0f)
        , Shininess(32.0f)
        , EmissiveColor(0.0f, 0.0f, 0.0f, 1.0f)
    {
    }

    // Factory methods for common materials
    static FMaterial Default()
    {
        return FMaterial();
    }

    static FMaterial Diffuse(const FColor& Color)
    {
        FMaterial mat;
        mat.DiffuseColor = Color;
        mat.SpecularColor = FColor(0.2f, 0.2f, 0.2f, 1.0f);
        mat.Shininess = 16.0f;
        return mat;
    }

    static FMaterial Glossy(const FColor& Color, float Shine = 64.0f)
    {
        FMaterial mat;
        mat.DiffuseColor = Color;
        mat.SpecularColor = FColor(1.0f, 1.0f, 1.0f, 1.0f);
        mat.Shininess = Shine;
        return mat;
    }

    static FMaterial Metal(const FColor& Color, float Shine = 128.0f)
    {
        FMaterial mat;
        mat.DiffuseColor = Color;
        mat.SpecularColor = Color;  // Metallic surfaces reflect their base color
        mat.Shininess = Shine;
        return mat;
    }
};

/**
 * FLightScene - Container for all lights in the scene
 * Manages lights for the renderer, separate from game objects
 * Designed for future deferred rendering light culling
 */
class FLightScene
{
public:
    FLightScene();
    ~FLightScene();

    // Light management
    void AddLight(FLight* Light);
    void RemoveLight(FLight* Light);
    void ClearLights();

    // Accessors
    const std::vector<FLight*>& GetLights() const { return Lights; }
    
    // Get lights by type (for forward rendering multiple passes or deferred light pass)
    std::vector<FDirectionalLight*> GetDirectionalLights() const;
    std::vector<FPointLight*> GetPointLights() const;

    // Ambient light for the scene (global illumination approximation)
    void SetAmbientLight(const FColor& Color) { AmbientLight = Color; }
    const FColor& GetAmbientLight() const { return AmbientLight; }

private:
    std::vector<FLight*> Lights;
    FColor AmbientLight;
};
