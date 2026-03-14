#include "DXGIUtils.h"
#include <cstring>

void SetupFlipModelSwapChainDesc(DXGI_SWAP_CHAIN_DESC1& sd) {
    memset(&sd, 0, sizeof(sd));
    sd.Width = 0;                               // Use window size
    sd.Height = 0;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Stereo = FALSE;
    sd.SampleDesc.Count = 1;                    // Flip model requires SampleDesc.Count = 1
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;                         // Flip model requires at least 2
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Modern flip model
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}
