# Performance Analysis: Python vs C++

A common question for real-time applications, especially in sim racing where latency is critical, is the choice of language.

## Python (Current Implementation)

*   **Pros**: Rapid development, readability, easy memory mapping (`mmap`), huge ecosystem of libraries.
*   **Cons**:
    *   **Garbage Collection (GC)**: Python's GC can introduce "stop-the-world" pauses. In a loop running at 400Hz (2.5ms per frame), a GC pause of 1-2ms is significant and can cause FFB stutter.
    *   **GIL (Global Interpreter Lock)**: Limits true parallelism, though less relevant for a single-threaded logic loop.
    *   **Interpreter Overhead**: Execution speed of math operations is slower than compiled C++.

## C/C++ (Potential Port)

*   **Pros**:
    *   **Deterministic Latency**: No GC. Manual memory management ensures consistent loop times.
    *   **Raw Speed**: Math operations are negligible in cost.
    *   **Direct Hardware Access**: Easier to interface directly with DirectInput or wheel drivers without wrappers like `pyvjoy`.
*   **Cons**: Slower development time, more complex build system.

## Impact on FFB

For FFB, **consistency** is more important than raw throughput.
*   If the Python loop jitters (e.g., takes 1ms one frame and 4ms the next), the FFB signal sent to the wheel will be uneven, feeling "grainy" or "notchy".
*   Moving to C++ would significantly reduce **jitter**, ensuring the FFB update happens exactly when the telemetry is fresh.
*   **Input Lag**: C++ would likely reduce end-to-end input lag by 2-5ms by removing interpreter overhead and wrapper layers (`pyvjoy`).

**Verdict**: For a production-grade FFB app that rivals iRFFB in "feel", a move to C++ (or Rust/C#) is highly recommended. Python is excellent for prototyping the logic (as done here), but the final low-level loop benefits greatly from a compiled language.
