# 鼠标和键盘控制实现总结

## 概述

本次实现为 UE5MinimalRenderer 添加了完整的 UE5 风格鼠标控制，允许用户通过鼠标与 3D 场景进行交互。

## 实现的功能

根据需求文档，已实现以下所有控制方式：

### 1. 按住 LMB（鼠标左键）+ 拖动鼠标
**功能**：前后移动摄像机，并左右旋转

**实现细节**：
- 垂直拖动（Y 轴）：控制摄像机前后移动
- 水平拖动（X 轴）：控制摄像机左右旋转（Yaw 旋转）

这允许用户在导航时自然地转向移动方向。

### 2. 按住 RMB（鼠标右键）+ 拖动鼠标
**功能**：旋转摄像机以环顾关卡

**实现细节**：
- 水平拖动（X 轴）：控制摄像机左右旋转（Yaw 旋转）
- 垂直拖动（Y 轴）：控制摄像机上下旋转（Pitch 旋转）

这是"自由查看"模式 - 旋转摄像机查看周围，但不移动位置。

### 3. 按住 LMB + 按住 RMB + 拖动鼠标 或 按住 MMB（鼠标中键）+ 拖动鼠标
**功能**：使摄像机上移、下移、左移或右移，且不旋转摄像机

**实现细节**：
- 水平拖动（X 轴）：摄像机左右平移
- 垂直拖动（Y 轴）：摄像机上下平移

摄像机位置改变但不旋转，适合调整框架而不改变视角。

### 4. 向上或向下滚动 MMB（鼠标滚轮）
**功能**：使摄像机逐渐前移或后移

**实现细节**：
- 向上滚动：摄像机向前移动（放大）
- 向下滚动：摄像机向后移动（缩小）

沿摄像机正前方方向快速缩放。

## 技术实现

### 代码修改

#### 1. Camera.h 和 Camera.cpp
扩展了 `FCamera` 类，添加了以下功能：

**新增成员变量**：
```cpp
float Pitch;        // 俯仰角（上下旋转）
float Yaw;          // 偏航角（左右旋转）
FVector Forward;    // 前向向量
FVector Right;      // 右向向量
FVector Up;         // 上向向量
```

**新增方法**：
```cpp
void MoveForwardBackward(float Delta);  // 前后移动
void RotateYaw(float Delta);           // 左右旋转
void RotatePitch(float Delta);         // 上下旋转
void PanRight(float Delta);            // 左右平移
void PanUp(float Delta);               // 上下平移
void Zoom(float Delta);                // 缩放
void UpdateOrientation();              // 更新摄像机方向
```

#### 2. Game.h 和 Game.cpp
添加了获取摄像机的接口：
```cpp
FCamera* GetCamera();  // 返回渲染器的摄像机指针
```

#### 3. Main.cpp
实现了完整的输入处理系统：

**输入状态跟踪**：
```cpp
struct FInputState {
    bool bLeftMouseDown;      // 左键按下
    bool bRightMouseDown;     // 右键按下
    bool bMiddleMouseDown;    // 中键按下
    int LastMouseX;           // 上次鼠标 X 位置
    int LastMouseY;           // 上次鼠标 Y 位置
};
```

**Windows 消息处理**：
- `WM_LBUTTONDOWN/UP`：处理左键按下/释放
- `WM_RBUTTONDOWN/UP`：处理右键按下/释放
- `WM_MBUTTONDOWN/UP`：处理中键按下/释放
- `WM_MOUSEMOVE`：计算鼠标增量并应用相应的摄像机变换
- `WM_MOUSEWHEEL`：根据滚轮方向应用缩放

**鼠标捕获管理**：
- 按下任意鼠标按钮时调用 `SetCapture()` 以捕获鼠标
- 所有按钮释放时调用 `ReleaseCapture()` 释放捕获

### 灵敏度设置

当前的灵敏度值（可在 Main.cpp 的 `CameraSettings` 命名空间中调整）：
```cpp
namespace CameraSettings {
    constexpr float MovementSpeed = 0.01f;   // 前后移动速度
    constexpr float RotationSpeed = 0.005f;  // 旋转速度
    constexpr float PanSpeed = 0.01f;        // 平移速度
    constexpr float ZoomSpeed = 0.5f;        // 缩放速度
}
```

## 使用方法

1. **启动应用程序**：将看到旋转的彩色立方体
2. **按住 RMB 并拖动**：从各个角度查看立方体
3. **按住 LMB 向上拖动**：向立方体靠近
4. **按住 LMB 左右拖动**：边移动边围绕立方体旋转
5. **按住 MMB 拖动**：在画面中重新定位立方体，不改变视角
6. **滚动鼠标滚轮**：快速放大/缩小

## 与 UE5 的对比

本实现紧密遵循 UE5 的视口控制方式，主要区别：

**已实现**：
- ✅ 所有基本鼠标控制（LMB、RMB、MMB、滚轮）
- ✅ 按钮组合（LMB + RMB）
- ✅ 鼠标捕获以支持窗口外拖动
- ✅ 平滑的增量控制

**简化之处**：
- 暂未实现键盘修饰键（Alt、Shift、Ctrl）
- 暂未实现围绕点旋转模式
- 简化的灵敏度设置（UE5 有更多自定义选项）

## 文档

创建了以下文档文件：

1. **CAMERA_CONTROLS.md**：英文版详细控制文档
   - 控制方式说明
   - 技术实现细节
   - 使用示例
   - 未来改进建议

2. **README.md**：更新了主文档
   - 添加了"Interactive Camera Controls"部分
   - 在功能列表中提到了交互控制

3. **CAMERA_CONTROLS_CN.md**（本文件）：中文版实现总结

## 测试要求

由于这是一个仅支持 Windows 的 DirectX 12 项目，需要在 Windows 环境下进行测试：

1. 使用 Visual Studio 2019 或更高版本编译项目
2. 运行生成的可执行文件
3. 测试所有鼠标控制功能
4. 验证控制响应流畅自然

## 代码质量

- ✅ 遵循现有代码风格（F 前缀类名、驼峰命名）
- ✅ 最小化代码修改（仅添加必要功能）
- ✅ 完整的注释说明
- ✅ 正确的资源管理（鼠标捕获/释放）
- ✅ 健壮的错误处理（空指针检查）

## 总结

本次实现完整地按照需求文档添加了 UE5 风格的鼠标控制功能，所有要求的操作方式均已实现并正确映射到摄像机控制方法。代码质量高，遵循项目现有架构和编码规范，为用户提供了直观自然的 3D 场景交互体验。
