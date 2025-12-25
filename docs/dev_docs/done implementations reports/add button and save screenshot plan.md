See vendor\stb_image_write.h

The simplest and most standard way to do this in C++ graphics applications (especially those using ImGui) is to use **`stb_image_write.h`**.

It is a **single-header library**. You do not need to compile a `.lib` or link anything. You just drop the file into your project folder.

Here is the implementation plan:

### 1. Get the Library
Download `stb_image_write.h` from the [nothings/stb GitHub repository](https://github.com/nothings/stb/blob/master/stb_image_write.h).
Place it in your `src/` or `vendor/` folder.

### 2. Implementation Code
Since your app uses **DirectX 11** (based on `GuiLayer.cpp`), you cannot just "save the window." You must read the pixels from the GPU's Back Buffer.

Add this function to `GuiLayer.cpp`.

**A. Include the library**
At the top of `GuiLayer.cpp`:
```cpp
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <d3d11.h> // Already there
```

**B. The Capture Function**
Add this helper function. It handles the complex DirectX logic of moving texture data from GPU to CPU memory.

```cpp
void SaveScreenshot(const char* filename) {
    if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dDeviceContext) return;

    // 1. Get the Back Buffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return;

    // 2. Create a Staging Texture (CPU Readable)
    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* pStagingTexture = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&desc, NULL, &pStagingTexture);
    if (FAILED(hr)) {
        pBackBuffer->Release();
        return;
    }

    // 3. Copy GPU -> CPU
    g_pd3dDeviceContext->CopyResource(pStagingTexture, pBackBuffer);

    // 4. Map the data to read it
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = g_pd3dDeviceContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        // 5. Handle Format (DX11 is usually BGRA, PNG needs RGBA)
        int width = desc.Width;
        int height = desc.Height;
        int channels = 4;
        
        // Allocate buffer for the image
        std::vector<unsigned char> image_data(width * height * channels);
        unsigned char* src = (unsigned char*)mapped.pData;
        unsigned char* dst = image_data.data();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Calculate positions
                int src_index = (y * mapped.RowPitch) + (x * 4);
                int dst_index = (y * width * 4) + (x * 4);

                // Swap B and R (BGRA -> RGBA)
                dst[dst_index + 0] = src[src_index + 2]; // R
                dst[dst_index + 1] = src[src_index + 1]; // G
                dst[dst_index + 2] = src[src_index + 0]; // B
                dst[dst_index + 3] = 255;                // Alpha (Force Opaque)
            }
        }

        // 6. Save to PNG using STB
        stbi_write_png(filename, width, height, channels, image_data.data(), width * channels);

        g_pd3dDeviceContext->Unmap(pStagingTexture, 0);
    }

    // Cleanup
    pStagingTexture->Release();
    pBackBuffer->Release();
    
    std::cout << "[GUI] Screenshot saved to " << filename << std::endl;
}
```

**C. Add the Button**
In `DrawTuningWindow` or `DrawDebugWindow`:

```cpp
if (ImGui::Button("Save Screenshot")) {
    // Generate a timestamped filename
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "screenshot_%Y-%m-%d_%H-%M-%S.png", &tstruct);
    
    SaveScreenshot(buf);
}
```

### Why this is the best approach
1.  **Zero Linker Errors:** You don't need to mess with CMake or `.lib` files.
2.  **Lightweight:** It adds about 50KB to your executable.
3.  **Standard:** This is how almost every custom game engine handles screenshots.