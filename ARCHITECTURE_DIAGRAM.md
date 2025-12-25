# Architecture Diagram: Text Rendering and Statistics

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         APPLICATION                              │
│                      (Runtime/Main.cpp)                          │
│  • Window management                                             │
│  • Main game loop                                                │
│  • Frame timing                                                  │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                         GAME LAYER                               │
│                        (Game/Game.cpp)                           │
│  • FGame::Tick(deltaTime)                                        │
│  • Game world management                                         │
│  • Triangle object (example)                                     │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       RENDERER LAYER                             │
│                    (Renderer/Renderer.cpp)                       │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ FRenderer::RenderFrame()                                  │  │
│  │  1. Stats.BeginFrame()         ← Start timing            │  │
│  │  2. RHICmdList->BeginFrame()                             │  │
│  │  3. RHICmdList->ClearRenderTarget()                      │  │
│  │  4. RenderScene()               ← 3D geometry            │  │
│  │     └─ Stats.AddTriangles()     ← Count triangles        │  │
│  │  5. RenderStats()               ← Text overlay NEW!      │  │
│  │     └─ DrawText() x 4           ← 4 stat lines           │  │
│  │  6. RHICmdList->EndFrame()                               │  │
│  │  7. RHICmdList->Present()                                │  │
│  │  8. Stats.EndFrame()            ← Calculate FPS/time     │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ FRenderStats (RenderStats.h/cpp)              NEW!       │  │
│  │  • FrameCount: uint64           (total frames)           │  │
│  │  • FPS: float                   (frames/second)          │  │
│  │  • FrameTimeMs: float           (milliseconds)           │  │
│  │  • TriangleCount: uint32        (per frame)              │  │
│  └───────────────────────────────────────────────────────────┘  │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       RHI INTERFACE                              │
│                        (RHI/RHI.h)                               │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ FRHICommandList (interface)                              │  │
│  │  • BeginFrame()                                          │  │
│  │  • EndFrame()                                            │  │
│  │  • ClearRenderTarget()                                   │  │
│  │  • SetPipelineState()                                    │  │
│  │  • SetVertexBuffer()                                     │  │
│  │  • DrawPrimitive()                                       │  │
│  │  • DrawText()                    ← NEW API!              │  │
│  │  • Present()                                             │  │
│  └───────────────────────────────────────────────────────────┘  │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    DX12 IMPLEMENTATION                           │
│                  (RHI_DX12/DX12RHI.cpp)                          │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ FDX12CommandList::DrawText() Implementation  NEW!        │  │
│  │                                                           │  │
│  │  1. AcquireWrappedResources()   ← Get D3D11 resource     │  │
│  │  2. D2DDeviceContext->SetTarget()                        │  │
│  │  3. D2DDeviceContext->BeginDraw()                        │  │
│  │  4. CreateTextFormat()          ← Font, size             │  │
│  │  5. CreateSolidColorBrush()     ← Color                  │  │
│  │  6. DrawTextW()                 ← Render text            │  │
│  │  7. EndDraw()                                            │  │
│  │  8. ReleaseWrappedResources()   ← Release D3D11         │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ Text Rendering Resources (NEW!)                          │  │
│  │  • ID3D11On12Device              ← DX11/DX12 bridge      │  │
│  │  • ID2D1DeviceContext2           ← Direct2D context      │  │
│  │  • IDWriteFactory                ← DirectWrite           │  │
│  │  • WrappedBackBuffers[2]         ← D3D11 wrapped RT      │  │
│  │  • D2DRenderTargets[2]           ← D2D bitmap RT         │  │
│  └───────────────────────────────────────────────────────────┘  │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                           GPU                                    │
│  • DirectX 12 pipeline      (3D rendering)                       │
│  • Direct2D pipeline        (Text rendering)  NEW!               │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow: Statistics Display

```
┌──────────────┐
│ Frame Start  │
└──────┬───────┘
       │
       ▼
┌─────────────────────────────┐
│ Stats.BeginFrame()          │
│  • Record start time        │
│  • Reset triangle count     │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│ Render 3D Scene             │
│  • For each proxy:          │
│    - Render geometry        │
│    - Stats.AddTriangles()   │◄── Accumulate triangle count
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────┐
│ RenderStats()                               │
│  • DrawText("Frame: 123")   ◄───────────────┼── Frame count
│  • DrawText("FPS: 60.0")    ◄───────────────┼── FPS (0.5s avg)
│  • DrawText("Frame Time...")◄───────────────┼── Current frame time
│  • DrawText("Triangles: 1") ◄───────────────┼── Triangle count
└──────┬──────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│ Stats.EndFrame()            │
│  • Calculate frame time     │
│  • Increment frame count    │
│  • Update FPS (if 0.5s)     │
└──────┬──────────────────────┘
       │
       ▼
┌──────────────┐
│  Frame End   │
└──────────────┘
```

## Text Rendering Pipeline (D3D11on12)

```
┌─────────────────────────────────────────────────────────────┐
│                    DirectX 12 Side                          │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ D3D12 Render Targets (2 back buffers)                │  │
│  │  • DXGI_FORMAT_R8G8B8A8_UNORM                        │  │
│  │  • State: RENDER_TARGET                              │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │                                   │
│                          │ Wrapped by D3D11On12Device        │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Wrapped Resources (D3D11/D3D12 bridge)               │  │
│  │  • ID3D11Resource                                    │  │
│  │  • Acquire/Release for interop                       │  │
│  └───────────────────────┬───────────────────────────────┘  │
└────────────────────────────┼───────────────────────────────┘
                             │
┌────────────────────────────┼───────────────────────────────┐
│                    Direct2D Side                            │
│                            ▼                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ D2D Bitmap Render Targets (from DXGI surface)        │  │
│  │  • ID2D1Bitmap1                                      │  │
│  │  • D2D1_BITMAP_OPTIONS_TARGET                        │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │                                   │
│                          │ Set as D2D target                 │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ ID2D1DeviceContext2                                  │  │
│  │  • BeginDraw()                                       │  │
│  │  • DrawTextW() ◄─── Text + Format + Brush           │  │
│  │  • EndDraw()                                         │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ IDWriteFactory                                       │  │
│  │  • CreateTextFormat(Arial, 18pt)                    │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ ID2D1SolidColorBrush                                 │  │
│  │  • Color: Yellow (1.0, 1.0, 0.0, 1.0)               │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Statistics Calculation

```
Frame Count:
  ┌─────────────┐
  │ FrameCount++│  Every frame
  └─────────────┘

FPS Calculation:
  ┌──────────────────────────────────────┐
  │ Every 0.5 seconds:                   │
  │ FPS = FramesSinceUpdate / 0.5        │
  │                                      │
  │ Example: 30 frames in 0.5s = 60 FPS │
  └──────────────────────────────────────┘

Frame Time:
  ┌──────────────────────────────────────┐
  │ CurrentTime - FrameStartTime         │
  │                                      │
  │ Example: 16.67 ms at 60 FPS          │
  └──────────────────────────────────────┘

Triangle Count:
  ┌──────────────────────────────────────┐
  │ Sum of all proxies' triangle counts  │
  │ TriangleCount += VertexCount / 3     │
  │                                      │
  │ Example: 3 vertices = 1 triangle     │
  └──────────────────────────────────────┘
```

## Component Relationships

```
┌─────────────────┐
│   FRenderer     │
│                 │
│  + Stats        │◄────┐
│  + RHI          │     │
└────┬────────────┘     │
     │                  │
     │ owns             │
     ▼                  │
┌─────────────────┐     │
│ FRenderStats    │     │
│                 │     │
│ - FrameCount    │─────┘ provides stats
│ - FPS           │
│ - FrameTimeMs   │
│ - TriangleCount │
└─────────────────┘

┌──────────────────┐
│ FRHICommandList  │◄──── interface
└────────┬─────────┘
         │
         │ implements
         ▼
┌──────────────────┐
│FDX12CommandList  │
│                  │
│ + D2DContext     │◄──── text rendering
│ + DWriteFactory  │
│ + WrappedBuffers │
└──────────────────┘
```

## Memory Layout

```
FRenderer (main renderer)
  │
  ├─ Stats: FRenderStats (~100 bytes)
  │   ├─ FrameCount: uint64 (8 bytes)
  │   ├─ FPS: float (4 bytes)
  │   ├─ FrameTimeMs: float (4 bytes)
  │   ├─ TriangleCount: uint32 (4 bytes)
  │   └─ Timing data (~80 bytes)
  │
  └─ RHI: FRHI*
      └─ CommandList: FDX12CommandList
          ├─ DX12 resources (~1 MB)
          └─ D2D/DWrite resources (~2 MB) NEW!
              ├─ D3D11On12Device
              ├─ D2DDeviceContext
              ├─ DWriteFactory
              ├─ WrappedBuffers[2]
              └─ D2DRenderTargets[2]

Total Memory Overhead: ~2.1 MB
```

## Performance Timeline (60 FPS = 16.67ms per frame)

```
Frame Timeline:
0.00 ms  │ BeginFrame()
         │ Stats.BeginFrame()        ◄── Start timing
         │
0.10 ms  │ ClearRenderTarget()
         │
0.20 ms  │ RenderScene()
         │  └─ DrawPrimitive()       ◄── 3D triangle
         │  └─ Stats.AddTriangles()  (~0.01 ms)
         │
8.00 ms  │ RenderStats()
         │  ├─ DrawText() x 4        (~0.1 ms each)
         │  └─ Total: ~0.4 ms        ◄── Text overlay
         │
8.50 ms  │ EndFrame()
         │
16.00 ms │ Present()                 ◄── VSync wait
         │ Stats.EndFrame()          (~0.01 ms)
16.67 ms │ Frame Complete

Text Rendering Overhead: 0.4 ms (2.4% of frame time)
Stats Tracking Overhead: 0.02 ms (0.1% of frame time)
Total Impact: ~2.5% of frame time
```
