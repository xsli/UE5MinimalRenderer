/**
 * Unit tests for matrix transformation functions
 * Tests the FMatrix4x4 class from CoreTypes.h
 */

#include <gtest/gtest.h>
#include "CoreTypes.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to compare floats with tolerance
bool FloatEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::abs(a - b) < epsilon;
}

// Helper function to extract matrix element using DirectXMath
float GetMatrixElement(const FMatrix4x4& mat, int row, int col)
{
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, mat.Matrix);
    return m.m[row][col];
}

// Helper function to compare two matrices
bool MatrixEqual(const FMatrix4x4& a, const FMatrix4x4& b, float epsilon = 0.0001f)
{
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (!FloatEqual(GetMatrixElement(a, row, col), GetMatrixElement(b, row, col), epsilon))
            {
                return false;
            }
        }
    }
    return true;
}

// Helper function to transform a point by a matrix
FVector TransformPoint(const FMatrix4x4& mat, const FVector& point)
{
    DirectX::XMVECTOR v = DirectX::XMVectorSet(point.X, point.Y, point.Z, 1.0f);
    DirectX::XMVECTOR result = DirectX::XMVector4Transform(v, mat.Matrix);
    DirectX::XMFLOAT4 r;
    DirectX::XMStoreFloat4(&r, result);
    // Perspective divide (if w != 1)
    if (std::abs(r.w) > 0.0001f)
    {
        return FVector(r.x / r.w, r.y / r.w, r.z / r.w);
    }
    return FVector(r.x, r.y, r.z);
}

// Test class
class MatrixTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code if needed
    }
};

// ============================================
// Identity Matrix Tests
// ============================================

TEST_F(MatrixTest, IdentityMatrix_DiagonalOnes)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    
    // Check diagonal elements are 1
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 0, 0), 1.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 1, 1), 1.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 2, 2), 1.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 3, 3), 1.0f));
}

TEST_F(MatrixTest, IdentityMatrix_OffDiagonalZeros)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    
    // Check off-diagonal elements are 0
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 0, 1), 0.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 0, 2), 0.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 1, 0), 0.0f));
    EXPECT_TRUE(FloatEqual(GetMatrixElement(identity, 2, 0), 0.0f));
}

TEST_F(MatrixTest, IdentityMatrix_PreservesPoint)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    FVector point(3.5f, -2.0f, 7.0f);
    
    FVector result = TransformPoint(identity, point);
    
    EXPECT_TRUE(FloatEqual(result.X, point.X));
    EXPECT_TRUE(FloatEqual(result.Y, point.Y));
    EXPECT_TRUE(FloatEqual(result.Z, point.Z));
}

// ============================================
// Translation Matrix Tests
// ============================================

TEST_F(MatrixTest, Translation_MovesPointCorrectly)
{
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, -3.0f, 2.0f);
    FVector point(1.0f, 1.0f, 1.0f);
    
    FVector result = TransformPoint(trans, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 6.0f));   // 1 + 5
    EXPECT_TRUE(FloatEqual(result.Y, -2.0f));  // 1 + (-3)
    EXPECT_TRUE(FloatEqual(result.Z, 3.0f));   // 1 + 2
}

TEST_F(MatrixTest, Translation_OriginMovesToTranslation)
{
    FMatrix4x4 trans = FMatrix4x4::Translation(10.0f, 20.0f, 30.0f);
    FVector origin(0.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(trans, origin);
    
    EXPECT_TRUE(FloatEqual(result.X, 10.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 20.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 30.0f));
}

TEST_F(MatrixTest, Translation_ZeroTranslation_PreservesPoint)
{
    FMatrix4x4 trans = FMatrix4x4::Translation(0.0f, 0.0f, 0.0f);
    FVector point(5.0f, -3.0f, 2.0f);
    
    FVector result = TransformPoint(trans, point);
    
    EXPECT_TRUE(FloatEqual(result.X, point.X));
    EXPECT_TRUE(FloatEqual(result.Y, point.Y));
    EXPECT_TRUE(FloatEqual(result.Z, point.Z));
}

// ============================================
// Rotation Matrix Tests
// ============================================

TEST_F(MatrixTest, RotationX_90Degrees_RotatesYToZ)
{
    // Rotate 90 degrees around X axis
    // Y axis should become Z axis, Z axis should become -Y axis
    FMatrix4x4 rotX = FMatrix4x4::RotationX(static_cast<float>(M_PI / 2.0));
    
    // Point on Y axis
    FVector pointOnY(0.0f, 1.0f, 0.0f);
    FVector result = TransformPoint(rotX, pointOnY);
    
    // Should now be on Z axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 1.0f));
}

TEST_F(MatrixTest, RotationX_90Degrees_RotatesZToNegY)
{
    FMatrix4x4 rotX = FMatrix4x4::RotationX(static_cast<float>(M_PI / 2.0));
    
    // Point on Z axis
    FVector pointOnZ(0.0f, 0.0f, 1.0f);
    FVector result = TransformPoint(rotX, pointOnZ);
    
    // Should now be on -Y axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, -1.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, RotationY_90Degrees_RotatesXToZ)
{
    // Rotate 90 degrees around Y axis
    FMatrix4x4 rotY = FMatrix4x4::RotationY(static_cast<float>(M_PI / 2.0));
    
    // Point on X axis
    FVector pointOnX(1.0f, 0.0f, 0.0f);
    FVector result = TransformPoint(rotY, pointOnX);
    
    // Should now be on -Z axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, -1.0f));
}

TEST_F(MatrixTest, RotationY_90Degrees_RotatesZToX)
{
    FMatrix4x4 rotY = FMatrix4x4::RotationY(static_cast<float>(M_PI / 2.0));
    
    // Point on Z axis
    FVector pointOnZ(0.0f, 0.0f, 1.0f);
    FVector result = TransformPoint(rotY, pointOnZ);
    
    // Should now be on X axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, 1.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, RotationZ_90Degrees_RotatesXToY)
{
    // Rotate 90 degrees around Z axis
    FMatrix4x4 rotZ = FMatrix4x4::RotationZ(static_cast<float>(M_PI / 2.0));
    
    // Point on X axis
    FVector pointOnX(1.0f, 0.0f, 0.0f);
    FVector result = TransformPoint(rotZ, pointOnX);
    
    // Should now be on Y axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 1.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, RotationZ_90Degrees_RotatesYToNegX)
{
    FMatrix4x4 rotZ = FMatrix4x4::RotationZ(static_cast<float>(M_PI / 2.0));
    
    // Point on Y axis
    FVector pointOnY(0.0f, 1.0f, 0.0f);
    FVector result = TransformPoint(rotZ, pointOnY);
    
    // Should now be on -X axis (left-handed system)
    EXPECT_TRUE(FloatEqual(result.X, -1.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, Rotation_360Degrees_ReturnsToOriginal)
{
    FMatrix4x4 rotX = FMatrix4x4::RotationX(static_cast<float>(2.0 * M_PI));
    FVector point(1.0f, 2.0f, 3.0f);
    
    FVector result = TransformPoint(rotX, point);
    
    EXPECT_TRUE(FloatEqual(result.X, point.X));
    EXPECT_TRUE(FloatEqual(result.Y, point.Y));
    EXPECT_TRUE(FloatEqual(result.Z, point.Z));
}

// ============================================
// Scaling Matrix Tests
// ============================================

TEST_F(MatrixTest, Scaling_UniformScale_MultipliesAll)
{
    FMatrix4x4 scale = FMatrix4x4::Scaling(2.0f, 2.0f, 2.0f);
    FVector point(1.0f, 2.0f, 3.0f);
    
    FVector result = TransformPoint(scale, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 2.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 4.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 6.0f));
}

TEST_F(MatrixTest, Scaling_NonUniformScale_ScalesIndependently)
{
    FMatrix4x4 scale = FMatrix4x4::Scaling(1.0f, 2.0f, 3.0f);
    FVector point(1.0f, 1.0f, 1.0f);
    
    FVector result = TransformPoint(scale, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 1.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 2.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 3.0f));
}

TEST_F(MatrixTest, Scaling_UnitScale_PreservesPoint)
{
    FMatrix4x4 scale = FMatrix4x4::Scaling(1.0f, 1.0f, 1.0f);
    FVector point(5.0f, -3.0f, 2.0f);
    
    FVector result = TransformPoint(scale, point);
    
    EXPECT_TRUE(FloatEqual(result.X, point.X));
    EXPECT_TRUE(FloatEqual(result.Y, point.Y));
    EXPECT_TRUE(FloatEqual(result.Z, point.Z));
}

TEST_F(MatrixTest, Scaling_ZeroScale_CollapsesToOrigin)
{
    FMatrix4x4 scale = FMatrix4x4::Scaling(0.0f, 0.0f, 0.0f);
    FVector point(5.0f, -3.0f, 2.0f);
    
    FVector result = TransformPoint(scale, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

// ============================================
// Matrix Multiplication Tests
// ============================================

TEST_F(MatrixTest, Multiplication_IdentityTimesAny_ReturnsOriginal)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, 3.0f, 2.0f);
    
    FMatrix4x4 result = identity * trans;
    
    EXPECT_TRUE(MatrixEqual(result, trans));
}

TEST_F(MatrixTest, Multiplication_AnyTimesIdentity_ReturnsOriginal)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, 3.0f, 2.0f);
    
    FMatrix4x4 result = trans * identity;
    
    EXPECT_TRUE(MatrixEqual(result, trans));
}

TEST_F(MatrixTest, Multiplication_TranslationOrder_Accumulates)
{
    FMatrix4x4 trans1 = FMatrix4x4::Translation(1.0f, 0.0f, 0.0f);
    FMatrix4x4 trans2 = FMatrix4x4::Translation(0.0f, 2.0f, 0.0f);
    
    FMatrix4x4 combined = trans1 * trans2;
    FVector origin(0.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(combined, origin);
    
    EXPECT_TRUE(FloatEqual(result.X, 1.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 2.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, Multiplication_ScaleThenTranslate_CorrectOrder)
{
    // Scale * Translation (scale happens first, then translate)
    // Point (1,0,0) -> scale by 2 -> (2,0,0) -> translate by (5,0,0) -> (7,0,0)
    FMatrix4x4 scale = FMatrix4x4::Scaling(2.0f, 2.0f, 2.0f);
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, 0.0f, 0.0f);
    
    // In row-major, left-to-right multiplication: Scale * Trans
    // Transformation order: Scale first, then Translate
    FMatrix4x4 combined = scale * trans;
    FVector point(1.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(combined, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 7.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, Multiplication_TranslateThenScale_CorrectOrder)
{
    // Translation * Scale (translate happens first, then scale)
    // Point (1,0,0) -> translate by (5,0,0) -> (6,0,0) -> scale by 2 -> (12,0,0)
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, 0.0f, 0.0f);
    FMatrix4x4 scale = FMatrix4x4::Scaling(2.0f, 2.0f, 2.0f);
    
    // In row-major, left-to-right multiplication: Trans * Scale
    // Transformation order: Translate first, then Scale
    FMatrix4x4 combined = trans * scale;
    FVector point(1.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(combined, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 12.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

// ============================================
// Transpose Tests
// ============================================

TEST_F(MatrixTest, Transpose_Identity_RemainsIdentity)
{
    FMatrix4x4 identity = FMatrix4x4::Identity();
    FMatrix4x4 transposed = identity.Transpose();
    
    EXPECT_TRUE(MatrixEqual(identity, transposed));
}

TEST_F(MatrixTest, Transpose_DoubleTranpose_ReturnsOriginal)
{
    FMatrix4x4 trans = FMatrix4x4::Translation(5.0f, 3.0f, 2.0f);
    FMatrix4x4 doubleTransposed = trans.Transpose().Transpose();
    
    EXPECT_TRUE(MatrixEqual(trans, doubleTransposed));
}

TEST_F(MatrixTest, Transpose_SwapsRowsAndColumns)
{
    FMatrix4x4 mat = FMatrix4x4::Translation(1.0f, 2.0f, 3.0f);
    FMatrix4x4 transposed = mat.Transpose();
    
    // Check that elements are swapped
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_TRUE(FloatEqual(
                GetMatrixElement(mat, row, col),
                GetMatrixElement(transposed, col, row)
            ));
        }
    }
}

// ============================================
// FTransform Tests
// ============================================

// Local FTransform definition for testing (mirrors the one in Primitive.h)
// This avoids dependency on the Game module which requires Windows-only headers
struct FTransformForTest
{
    FVector Position;
    FVector Rotation;  // Euler angles in radians
    FVector Scale;

    FTransformForTest()
        : Position(0.0f, 0.0f, 0.0f)
        , Rotation(0.0f, 0.0f, 0.0f)
        , Scale(1.0f, 1.0f, 1.0f)
    {
    }

    // Get transformation matrix
    FMatrix4x4 GetMatrix() const
    {
        FMatrix4x4 scale = FMatrix4x4::Scaling(Scale.X, Scale.Y, Scale.Z);
        FMatrix4x4 rotationX = FMatrix4x4::RotationX(Rotation.X);
        FMatrix4x4 rotationY = FMatrix4x4::RotationY(Rotation.Y);
        FMatrix4x4 rotationZ = FMatrix4x4::RotationZ(Rotation.Z);
        FMatrix4x4 translation = FMatrix4x4::Translation(Position.X, Position.Y, Position.Z);

        // Standard graphics transformation order: Scale -> Rotate -> Translate
        // With DirectXMath row-vector convention (v * M), transformations are applied
        // left-to-right in the multiplication chain: Scale * Rot * Trans
        // Rotation order: X*Y*Z (Roll*Pitch*Yaw for Euler angles)
        return scale * rotationX * rotationY * rotationZ * translation;
    }
};

TEST_F(MatrixTest, FTransform_DefaultIsIdentityTransform)
{
    FTransformForTest transform;
    FMatrix4x4 mat = transform.GetMatrix();
    FVector point(1.0f, 2.0f, 3.0f);
    
    FVector result = TransformPoint(mat, point);
    
    // Default transform should preserve points
    EXPECT_TRUE(FloatEqual(result.X, point.X));
    EXPECT_TRUE(FloatEqual(result.Y, point.Y));
    EXPECT_TRUE(FloatEqual(result.Z, point.Z));
}

TEST_F(MatrixTest, FTransform_TranslationOnly)
{
    FTransformForTest transform;
    transform.Position = FVector(10.0f, 5.0f, -3.0f);
    
    FMatrix4x4 mat = transform.GetMatrix();
    FVector origin(0.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(mat, origin);
    
    EXPECT_TRUE(FloatEqual(result.X, 10.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 5.0f));
    EXPECT_TRUE(FloatEqual(result.Z, -3.0f));
}

TEST_F(MatrixTest, FTransform_ScaleOnly)
{
    FTransformForTest transform;
    transform.Scale = FVector(2.0f, 3.0f, 4.0f);
    
    FMatrix4x4 mat = transform.GetMatrix();
    FVector point(1.0f, 1.0f, 1.0f);
    
    FVector result = TransformPoint(mat, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 2.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 3.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 4.0f));
}

TEST_F(MatrixTest, FTransform_RotationOnly_Y90)
{
    FTransformForTest transform;
    transform.Rotation = FVector(0.0f, static_cast<float>(M_PI / 2.0), 0.0f);
    
    FMatrix4x4 mat = transform.GetMatrix();
    FVector pointOnX(1.0f, 0.0f, 0.0f);
    
    FVector result = TransformPoint(mat, pointOnX);
    
    // X axis rotated 90 degrees around Y should become -Z axis
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, -1.0f));
}

TEST_F(MatrixTest, FTransform_ScaleRotateTranslate_Order)
{
    FTransformForTest transform;
    transform.Position = FVector(10.0f, 0.0f, 0.0f);  // Translate
    transform.Rotation = FVector(0.0f, 0.0f, 0.0f);   // No rotation
    transform.Scale = FVector(2.0f, 2.0f, 2.0f);      // Scale
    
    FMatrix4x4 mat = transform.GetMatrix();
    FVector point(1.0f, 0.0f, 0.0f);
    
    // Order: Scale -> Rotate -> Translate
    // Point (1,0,0) -> scale 2x -> (2,0,0) -> translate (10,0,0) -> (12,0,0)
    FVector result = TransformPoint(mat, point);
    
    EXPECT_TRUE(FloatEqual(result.X, 12.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

// ============================================
// View Matrix Tests
// ============================================

TEST_F(MatrixTest, LookAtLH_CameraAtOriginLookingAtZ)
{
    // Camera at origin, looking down +Z axis
    FVector eye(0.0f, 0.0f, 0.0f);
    FVector target(0.0f, 0.0f, 1.0f);
    FVector up(0.0f, 1.0f, 0.0f);
    
    FMatrix4x4 view = FMatrix4x4::LookAtLH(eye, target, up);
    
    // Point on Z axis should remain on Z axis (in view space)
    FVector pointOnZ(0.0f, 0.0f, 5.0f);
    FVector result = TransformPoint(view, pointOnZ);
    
    // In view space, Z is forward
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 5.0f));
}

TEST_F(MatrixTest, LookAtLH_CameraBehindOrigin)
{
    // Camera at (0, 0, -5), looking at origin
    FVector eye(0.0f, 0.0f, -5.0f);
    FVector target(0.0f, 0.0f, 0.0f);
    FVector up(0.0f, 1.0f, 0.0f);
    
    FMatrix4x4 view = FMatrix4x4::LookAtLH(eye, target, up);
    
    // Origin should be at (0, 0, 5) in view space (5 units in front of camera)
    FVector origin(0.0f, 0.0f, 0.0f);
    FVector result = TransformPoint(view, origin);
    
    EXPECT_TRUE(FloatEqual(result.X, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(result.Z, 5.0f));
}

// ============================================
// Perspective Projection Tests
// ============================================

TEST_F(MatrixTest, PerspectiveFovLH_PointAtNearPlane)
{
    float fov = DirectX::XM_PIDIV4;  // 45 degrees
    float aspect = 16.0f / 9.0f;
    float nearZ = 1.0f;
    float farZ = 100.0f;
    
    FMatrix4x4 proj = FMatrix4x4::PerspectiveFovLH(fov, aspect, nearZ, farZ);
    
    // Point at near plane, center
    FVector pointNear(0.0f, 0.0f, nearZ);
    FVector result = TransformPoint(proj, pointNear);
    
    // Should be at Z = 0 in NDC (after perspective divide)
    EXPECT_TRUE(FloatEqual(result.Z, 0.0f));
}

TEST_F(MatrixTest, PerspectiveFovLH_PointAtFarPlane)
{
    float fov = DirectX::XM_PIDIV4;  // 45 degrees
    float aspect = 16.0f / 9.0f;
    float nearZ = 1.0f;
    float farZ = 100.0f;
    
    FMatrix4x4 proj = FMatrix4x4::PerspectiveFovLH(fov, aspect, nearZ, farZ);
    
    // Point at far plane, center
    FVector pointFar(0.0f, 0.0f, farZ);
    FVector result = TransformPoint(proj, pointFar);
    
    // Should be at Z = 1 in NDC (after perspective divide)
    EXPECT_TRUE(FloatEqual(result.Z, 1.0f));
}

// ============================================
// Combined MVP Tests
// ============================================

TEST_F(MatrixTest, MVP_TransformChain_CorrectlyChained)
{
    // Model transform: translate to (5, 0, 10)
    FMatrix4x4 model = FMatrix4x4::Translation(5.0f, 0.0f, 10.0f);
    
    // View: Camera at (0, 0, -5) looking at origin
    FMatrix4x4 view = FMatrix4x4::LookAtLH(
        FVector(0.0f, 0.0f, -5.0f),
        FVector(0.0f, 0.0f, 0.0f),
        FVector(0.0f, 1.0f, 0.0f)
    );
    
    // Combined MV
    FMatrix4x4 mv = model * view;
    
    // Origin of model (0,0,0) should be at (5, 0, 15) in view space
    // (5 units right, 15 units in front = 10 + 5)
    FVector modelOrigin(0.0f, 0.0f, 0.0f);
    FVector viewSpace = TransformPoint(mv, modelOrigin);
    
    EXPECT_TRUE(FloatEqual(viewSpace.X, 5.0f));
    EXPECT_TRUE(FloatEqual(viewSpace.Y, 0.0f));
    EXPECT_TRUE(FloatEqual(viewSpace.Z, 15.0f));
}

// Main function for running all tests
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
