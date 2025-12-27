#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include "Light.h"
#include <vector>

/**
 * Light visualization geometry generators
 * Creates wireframe representations of lights for debug/editor visualization
 * UE5-style light icons and range indicators
 */
class FLightVisualization
{
public:
    /**
     * Generate wireframe arrow for directional light
     * Arrow pointing in light direction, displayed at given position
     */
    static void GenerateDirectionalLightGeometry(
        const FVector& Direction,
        const FColor& Color,
        float ArrowLength,
        std::vector<FVertex>& OutVertices,
        std::vector<uint32>& OutIndices);
    
    /**
     * Generate wireframe sphere for point light
     * Shows light position and radius
     */
    static void GeneratePointLightGeometry(
        float Radius,
        const FColor& Color,
        uint32 Segments,
        std::vector<FVertex>& OutVertices,
        std::vector<uint32>& OutIndices);
    
    /**
     * Generate small cross/star marker for light position
     * Simple 3D cross to show light origin
     */
    static void GenerateLightMarker(
        const FColor& Color,
        float Size,
        std::vector<FVertex>& OutVertices,
        std::vector<uint32>& OutIndices);
};
