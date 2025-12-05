# Performance Analysis: Python vs C++

A common question for real-time applications, especially in sim racing where latency is critical, is the choice of language.

## Python (Legacy Prototype)

*   **Pros**: Rapid development, readability, easy memory mapping (`mmap`).
*   **Cons**:
    *   **Garbage Collection (GC)**: Python's GC can introduce "stop-the-world" pauses. In a loop running at 400Hz (2.5ms per frame), a GC pause of 1-2ms is significant.
    *   **Jitter**: The wake-up time of `time.sleep()` in Python is less consistent on Windows.

## C++ (Current Implementation)

*   **Pros**:
    *   **Deterministic Latency**: No GC. Manual memory management ensures consistent loop times.
    *   **Raw Speed**: Math operations are negligible in cost.
    *   **Multithreading**: We now use a dedicated `FFBThread` running at 400Hz, completely decoupled from the GUI thread.
    *   **Direct API Access**: Interfacing with Windows APIs (`OpenFileMapping`, `vJoyInterface`) is native.

## Technology Choice: C++ vs Rust

While Rust offers memory safety guarantees, **C++** was chosen for this project because:
1.  **Industry Standard**: The rFactor 2 plugin SDK is written in C++. Integration with `ctypes` structs (which mirror C structs) is trivial.
2.  **Examples**: Abundant sample code exists for rFactor 2 plugins in C++.
3.  **Dependencies**: The vJoy SDK is provided as a C/C++ library (`.lib`). Linking this in Rust requires FFI bindings which adds complexity.

## Benchmark Expectations

| Metric | Python | C++ |
| :--- | :--- | :--- |
| **Loop Rate** | ~350-400Hz (Jittery) | **400Hz (Stable)** |
| **Input Lag** | ~5-10ms added | **<1ms added** |
| **CPU Usage** | Moderate (Interpreter) | **Low (Compiled)** |
