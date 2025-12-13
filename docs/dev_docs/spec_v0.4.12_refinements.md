# Technical Specification: Refinements v0.4.12

## 1. Physics Tuning

### Grip Calculation
*   **Old:** `excess = max(0, slip - 0.15); grip = 1.0 - (excess * 2.0);`
*   **New:** `excess = max(0, slip - 0.10); grip = 1.0 - (excess * 4.0);`
*   **Rationale:** Modern GT cars peak around 0.08-0.10 rad. The old threshold was too loose.

## 2. GUI Reorganization Plan

**Header A: FFB Components (Output)**
*   *Main Forces:* Total Output, Base Torque, SoP (Base Chassis G), Rear Align Torque, Scrub Drag Force.
*   *Modifiers:* Oversteer Boost, Understeer Cut, Clipping.
*   *Textures:* Road, Slide, Lockup, Spin, Bottoming.

**Header B: Internal Physics (Brain)**
*   *Loads:* Calc Load (Front/Rear).
*   *Grip/Slip:* Calc Front Grip, Calc Rear Grip, Calc Front Slip Ratio, Front Slip Angle (Smooth), Rear Slip Angle (Smooth).
*   *Forces:* **Calc Rear Lat Force** (Moved from Input).

**Header C: Raw Game Telemetry (Input)**
*   *Driver:* Steering Torque, Steering Input (Angle), Combined Input (Thr/Brk).
*   *Vehicle:* Chassis Lat Accel, Car Speed.
*   *Raw Tire:* Raw Front Load, Raw Front Grip, Raw Rear Grip.
*   *Raw Physics:* Raw Front Slip Ratio, Raw Front Susp Force, Raw Front Ride Height.
*   *Velocities:* Avg Front Lat PatchVel, Avg Rear Lat PatchVel, Avg Front Long PatchVel, Avg Rear Long PatchVel.

## 3. Screenshot Implementation (`GuiLayer.cpp`)

**Include:**
```cpp
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // User must provide this file
#include <vector>
#include <ctime>
```

**Helper Function:**
```cpp
void SaveScreenshot(const char* filename) {
    if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dDeviceContext) return;
    
    // 1. Get Back Buffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return;

    // 2. Create Staging Texture (CPU Read)
    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* pStaging = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&desc, NULL, &pStaging);
    if (FAILED(hr)) { pBackBuffer->Release(); return; }

    // 3. Copy & Map
    g_pd3dDeviceContext->CopyResource(pStaging, pBackBuffer);
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(g_pd3dDeviceContext->Map(pStaging, 0, D3D11_MAP_READ, 0, &mapped))) {
        int w = desc.Width;
        int h = desc.Height;
        std::vector<unsigned char> data(w * h * 4);
        unsigned char* src = (unsigned char*)mapped.pData;
        
        // Copy row by row (handling pitch) and swizzle BGRA -> RGBA
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int s_idx = (y * mapped.RowPitch) + (x * 4);
                int d_idx = (y * w * 4) + (x * 4);
                data[d_idx + 0] = src[s_idx + 2]; // R
                data[d_idx + 1] = src[s_idx + 1]; // G
                data[d_idx + 2] = src[s_idx + 0]; // B
                data[d_idx + 3] = 255;            // A
            }
        }
        stbi_write_png(filename, w, h, 4, data.data(), w * 4);
        g_pd3dDeviceContext->Unmap(pStaging, 0);
        std::cout << "[GUI] Screenshot saved: " << filename << std::endl;
    }
    pStaging->Release();
    pBackBuffer->Release();
}
```