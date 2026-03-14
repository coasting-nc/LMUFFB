#ifndef DXGIUTILS_H
#define DXGIUTILS_H

#include <windows.h>

#ifdef _WIN32
#include <dxgi1_2.h>
#endif

// Helper to configure the modern DXGI Flip Model swap chain descriptor
void SetupFlipModelSwapChainDesc(DXGI_SWAP_CHAIN_DESC1& sd);

#endif // DXGIUTILS_H
