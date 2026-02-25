# Implementation Plan - Issue #189: Modernize DXGI Swap Chain to Flip Model

**Issue:** #189 - Legacy bitblt swap model vs. flip model conflict and DXGI deprecation

## Context
The current implementation of the GUI in LMUFFB uses a legacy DXGI bitblt swap model (`DXGI_SWAP_EFFECT_DISCARD`) and deprecated creation methods (`D3D11CreateDeviceAndSwapChain`). This is suboptimal for modern Windows (10/11) and can lead to performance issues, higher latency, and lack of support for modern display features like Variable Refresh Rate (VRR) and independent flip.

The goal is to modernize the DXGI initialization to use the Flip model (`DXGI_SWAP_EFFECT_FLIP_DISCARD`) and modern DXGI interfaces.

## Reference Documents
- [Microsoft Docs: DXGI Flip Model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)
- [Microsoft Docs: For best performance, use DXGI flip model](https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains)

## Codebase Analysis Summary
### Current Architecture Overview
- `src/GuiLayer_Win32.cpp` handles the Win32 window and D3D11/DXGI lifecycle.
- It uses `D3D11CreateDeviceAndSwapChain` which is a convenience function that bundles device and swap chain creation but uses legacy descriptors.

### Impacted Functionalities
- **D3D11/DXGI Initialization (`CreateDeviceD3D`)**: Will be refactored to use a more granular and modern approach.
- **Cleanup (`CleanupDeviceD3D`)**: Will be updated to release any new interfaces used.
- **Window Resizing (`WndProc` / `WM_SIZE`)**: The Flip model is stricter about releasing all back buffer references before resizing. The current `CleanupRenderTarget` should already handle this, but it will be verified.

## FFB Effect Impact Analysis
- **None**: This change is limited to the GUI presentation layer and does not affect FFB physics, telemetry processing, or output torque calculations.

## Proposed Changes
### `src/GuiLayer_Win32.cpp`
- **Refactor `CreateDeviceD3D`**:
    - Replace `D3D11CreateDeviceAndSwapChain` with `D3D11CreateDevice`.
    - Manually traverse the DXGI object hierarchy to get `IDXGIFactory2`.
    - Use `DXGI_SWAP_CHAIN_DESC1` to describe the swap chain.
    - Set `SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD`.
    - Set `BufferCount = 2` (minimum for flip model).
    - Set `Scaling = DXGI_SCALING_STRETCH`.
    - Call `CreateSwapChainForHwnd`.
- **Update `CleanupDeviceD3D`**:
    - Ensure `g_pSwapChain` (which will still be `IDXGISwapChain*` for compatibility) is released.

### Versioning
- Version will be incremented from `v0.7.80` to `v0.7.81`.

## Test Plan
- **Mock-based Unit Testing**:
    - Enhance `tests/linux_mock/windows.h` with D3D11 and DXGI mocks.
    - Implement `tests/test_dxgi_modernization.cpp` to verify that `CreateDeviceD3D` (when built in a test environment) correctly configures the `DXGI_SWAP_CHAIN_DESC1` structure.
    - Assert that `SwapEffect` is `DXGI_SWAP_EFFECT_FLIP_DISCARD`.
    - Assert that `BufferCount` is 2.
    - Assert that `SampleDesc.Count` is 1.
- **Static Analysis & Logic Verification**:
    - Verify that the HRESULT of each DXGI/D3D call is checked.
    - Verify that temporary interfaces (Factory, Adapter, Device) are released after swap chain creation.
- **Linux Build Verification**:
    - Ensure the project still compiles on Linux with `BUILD_HEADLESS=ON`.
- **Functional Verification (Manual/Visual on Windows - post-implementation)**:
    - Verify the GUI still renders correctly.
    - Verify window resizing works without crashing or D3D errors.

## Deliverables
- [x] Modified `src/GuiLayer_Win32.cpp`
- [x] New `src/DXGIUtils.h` and `src/DXGIUtils.cpp`
- [x] Updated `VERSION` and `Version.h` (via CMake)
- [x] Updated `USER_CHANGELOG.md` and `CHANGELOG_DEV.md`
- [x] New regression tests in `tests/test_dxgi_modernization.cpp`
- [x] This Implementation Plan updated with notes.

## Implementation Notes (to be filled after development)
- **Unforeseen Issues**:
    - The Linux build requires careful handling of the `DXGI_SWAP_CHAIN_DESC1` type. Even though it's Windows-specific, the core library needs to be able to compile it for unit tests. This was resolved by ensuring the `LinuxMock.h` provides all necessary definitions.
    - Some tests were generating leftover files (`old_version.ini`, `test_expansion.log`) in the repository root. These were addressed by adding explicit cleanup logic to the relevant test cases.
- **Plan Deviations**:
    - Extracted the swap chain configuration logic into `src/DXGIUtils.cpp` and `src/DXGIUtils.h`. This improved modularity and enabled a more meaningful regression test on Linux by calling the actual production logic rather than replicating it in the test body.
    - Enhanced `LinuxMock.h` to capture the swap chain descriptor during mock calls, allowing verification that the production code correctly passes parameters to the DXGI API.
- **Challenges**:
    - Mocking COM-style interfaces on Linux requires careful virtual function table alignment (though for these mocks, simple inheritance sufficed).
- **Recommendations**:
    - Future GUI-related changes should continue using the `DXGIUtils` pattern to maintain high test coverage on non-Windows platforms.
