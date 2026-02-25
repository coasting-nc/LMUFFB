# Code Coverage Report - v0.7.79

## Date: 2026-02-25

## 1. Executive Summary
This report summarizes the test coverage and verification results for LMUFFB v0.7.79 on Linux. The project maintains high coverage in core physics and configuration logic.

## 2. Test Execution Summary
- **C++ Tests**: 283/283 passed (1197 assertions)
- **Python Tests**: 18/18 passed

## 3. Core Physics Coverage (src/)
The following core files achieve excellent coverage:
- **FFBEngine.cpp**: 99.8%
- **GripLoadEstimation.cpp**: 100%
- **Config.h**: 100%
- **MathUtils.h**: 100%
- **PerfStats.h**: 100%
- **RateMonitor.h**: 100%
- **SteeringUtils.cpp**: 100%
- **VehicleUtils.cpp**: 98.4%

## 4. Coverage by File (Summary)
| File | Coverage % | Missing Lines |
| :--- | :--- | :--- |
| `src/AsyncLogger.h` | 96.6% | 147, 149, 223, 261-262 |
| `src/Config.cpp` | 91.5% | (Various UI/IO error paths) |
| `src/FFBEngine.cpp` | 99.8% | 61 |
| `src/GripLoadEstimation.cpp` | 100% | - |
| `src/VehicleUtils.cpp` | 98.4% | 46 |

## 5. UI and Platform Coverage Note
- **GuiLayer_Common.cpp**: 4.2% (Expected, as tests run in headless mode)
- **DirectInputFFB.cpp**: 33.3% (Hardware-dependent paths skipped in Linux mock)
- **main.cpp**: 0.0% (Application entry point skipped in unit tests)

## 6. Verification Environment
- **OS**: Linux (Headless)
- **Compiler**: GCC 13.3.0
- **Coverage Tool**: gcovr 8.6

## 7. Artifacts Location
Full reports are available in: `docs/dev_docs/reports/coverage/v0.7.79/`
- [HTML Report](coverage/v0.7.79/coverage.html)
- [XML (Cobertura)](coverage/v0.7.79/cobertura.xml)
- [JSON Report](coverage/v0.7.79/coverage.json)
- [CSV Report](coverage/v0.7.79/coverage.csv)
- [AI Summary](coverage/v0.7.79/ai_summary.txt)
