#include "LightVisualization.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void FLightVisualization::GenerateDirectionalLightGeometry(
    const FVector& Direction,
    const FColor& Color,
    float ArrowLength,
    std::vector<FVertex>& OutVertices,
    std::vector<uint32>& OutIndices)
{
    OutVertices.clear();
    OutIndices.clear();
    
    // Normalize direction
    float len = std::sqrt(Direction.X * Direction.X + Direction.Y * Direction.Y + Direction.Z * Direction.Z);
    if (len < 0.0001f) return;
    
    FVector dir(Direction.X / len, Direction.Y / len, Direction.Z / len);
    
    // Find perpendicular vectors
    FVector perp1, perp2;
    if (std::abs(dir.Y) < 0.9f) {
        perp1 = FVector(-dir.Z, 0.0f, dir.X);
    } else {
        perp1 = FVector(0.0f, dir.Z, -dir.Y);
    }
    float p1len = std::sqrt(perp1.X * perp1.X + perp1.Y * perp1.Y + perp1.Z * perp1.Z);
    perp1 = FVector(perp1.X / p1len, perp1.Y / p1len, perp1.Z / p1len);
    
    perp2 = FVector(
        dir.Y * perp1.Z - dir.Z * perp1.Y,
        dir.Z * perp1.X - dir.X * perp1.Z,
        dir.X * perp1.Y - dir.Y * perp1.X
    );
    
    // Origin point
    FVector origin(0.0f, 0.0f, 0.0f);
    
    // Arrow tip
    FVector tip(dir.X * ArrowLength, dir.Y * ArrowLength, dir.Z * ArrowLength);
    
    // Arrow head base (80% along arrow)
    float headBase = ArrowLength * 0.7f;
    FVector headPoint(dir.X * headBase, dir.Y * headBase, dir.Z * headBase);
    
    float headWidth = ArrowLength * 0.15f;
    
    // Main shaft line
    OutVertices.push_back({ origin, Color });
    OutVertices.push_back({ tip, Color });
    OutIndices.push_back(0);
    OutIndices.push_back(1);
    
    // Arrow head lines (4 lines forming a cone shape)
    FVector head1(headPoint.X + perp1.X * headWidth, headPoint.Y + perp1.Y * headWidth, headPoint.Z + perp1.Z * headWidth);
    FVector head2(headPoint.X - perp1.X * headWidth, headPoint.Y - perp1.Y * headWidth, headPoint.Z - perp1.Z * headWidth);
    FVector head3(headPoint.X + perp2.X * headWidth, headPoint.Y + perp2.Y * headWidth, headPoint.Z + perp2.Z * headWidth);
    FVector head4(headPoint.X - perp2.X * headWidth, headPoint.Y - perp2.Y * headWidth, headPoint.Z - perp2.Z * headWidth);
    
    uint32 tipIdx = 1;
    OutVertices.push_back({ head1, Color });
    OutVertices.push_back({ head2, Color });
    OutVertices.push_back({ head3, Color });
    OutVertices.push_back({ head4, Color });
    
    OutIndices.push_back(tipIdx); OutIndices.push_back(2);  // tip to head1
    OutIndices.push_back(tipIdx); OutIndices.push_back(3);  // tip to head2
    OutIndices.push_back(tipIdx); OutIndices.push_back(4);  // tip to head3
    OutIndices.push_back(tipIdx); OutIndices.push_back(5);  // tip to head4
    
    // Connect head base
    OutIndices.push_back(2); OutIndices.push_back(4);
    OutIndices.push_back(4); OutIndices.push_back(3);
    OutIndices.push_back(3); OutIndices.push_back(5);
    OutIndices.push_back(5); OutIndices.push_back(2);
    
    // Add parallel lines showing light rays
    float rayOffset = ArrowLength * 0.3f;
    float rayLength = ArrowLength * 0.4f;
    
    for (int i = 0; i < 4; ++i)
    {
        FVector offset;
        switch (i)
        {
            case 0: offset = FVector(perp1.X * rayOffset, perp1.Y * rayOffset, perp1.Z * rayOffset); break;
            case 1: offset = FVector(-perp1.X * rayOffset, -perp1.Y * rayOffset, -perp1.Z * rayOffset); break;
            case 2: offset = FVector(perp2.X * rayOffset, perp2.Y * rayOffset, perp2.Z * rayOffset); break;
            case 3: offset = FVector(-perp2.X * rayOffset, -perp2.Y * rayOffset, -perp2.Z * rayOffset); break;
        }
        
        FVector rayStart(offset.X, offset.Y, offset.Z);
        FVector rayEnd(offset.X + dir.X * rayLength, offset.Y + dir.Y * rayLength, offset.Z + dir.Z * rayLength);
        
        uint32 startIdx = OutVertices.size();
        OutVertices.push_back({ rayStart, Color });
        OutVertices.push_back({ rayEnd, Color });
        OutIndices.push_back(startIdx);
        OutIndices.push_back(startIdx + 1);
    }
}

void FLightVisualization::GeneratePointLightGeometry(
    float Radius,
    const FColor& Color,
    uint32 Segments,
    std::vector<FVertex>& OutVertices,
    std::vector<uint32>& OutIndices)
{
    OutVertices.clear();
    OutIndices.clear();
    
    // Generate three circles (XY, XZ, YZ planes) to show light radius
    
    // XY plane circle (horizontal)
    uint32 baseIdx = 0;
    for (uint32 i = 0; i <= Segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(Segments);
        float x = Radius * cosf(theta);
        float z = Radius * sinf(theta);
        OutVertices.push_back({ FVector(x, 0.0f, z), Color });
        
        if (i > 0)
        {
            OutIndices.push_back(baseIdx + i - 1);
            OutIndices.push_back(baseIdx + i);
        }
    }
    
    // XZ plane circle (vertical, facing camera default)
    baseIdx = OutVertices.size();
    for (uint32 i = 0; i <= Segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(Segments);
        float x = Radius * cosf(theta);
        float y = Radius * sinf(theta);
        OutVertices.push_back({ FVector(x, y, 0.0f), Color });
        
        if (i > 0)
        {
            OutIndices.push_back(baseIdx + i - 1);
            OutIndices.push_back(baseIdx + i);
        }
    }
    
    // YZ plane circle (vertical, side view)
    baseIdx = OutVertices.size();
    for (uint32 i = 0; i <= Segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(Segments);
        float y = Radius * cosf(theta);
        float z = Radius * sinf(theta);
        OutVertices.push_back({ FVector(0.0f, y, z), Color });
        
        if (i > 0)
        {
            OutIndices.push_back(baseIdx + i - 1);
            OutIndices.push_back(baseIdx + i);
        }
    }
}

void FLightVisualization::GenerateLightMarker(
    const FColor& Color,
    float Size,
    std::vector<FVertex>& OutVertices,
    std::vector<uint32>& OutIndices)
{
    OutVertices.clear();
    OutIndices.clear();
    
    // 3D cross/star marker
    // X axis
    OutVertices.push_back({ FVector(-Size, 0.0f, 0.0f), Color });
    OutVertices.push_back({ FVector(Size, 0.0f, 0.0f), Color });
    OutIndices.push_back(0);
    OutIndices.push_back(1);
    
    // Y axis
    OutVertices.push_back({ FVector(0.0f, -Size, 0.0f), Color });
    OutVertices.push_back({ FVector(0.0f, Size, 0.0f), Color });
    OutIndices.push_back(2);
    OutIndices.push_back(3);
    
    // Z axis
    OutVertices.push_back({ FVector(0.0f, 0.0f, -Size), Color });
    OutVertices.push_back({ FVector(0.0f, 0.0f, Size), Color });
    OutIndices.push_back(4);
    OutIndices.push_back(5);
    
    // Diagonal lines for more visibility
    float d = Size * 0.7f;
    OutVertices.push_back({ FVector(-d, -d, -d), Color });
    OutVertices.push_back({ FVector(d, d, d), Color });
    OutIndices.push_back(6);
    OutIndices.push_back(7);
    
    OutVertices.push_back({ FVector(-d, -d, d), Color });
    OutVertices.push_back({ FVector(d, d, -d), Color });
    OutIndices.push_back(8);
    OutIndices.push_back(9);
    
    OutVertices.push_back({ FVector(-d, d, -d), Color });
    OutVertices.push_back({ FVector(d, -d, d), Color });
    OutIndices.push_back(10);
    OutIndices.push_back(11);
    
    OutVertices.push_back({ FVector(d, -d, -d), Color });
    OutVertices.push_back({ FVector(-d, d, d), Color });
    OutIndices.push_back(12);
    OutIndices.push_back(13);
}
