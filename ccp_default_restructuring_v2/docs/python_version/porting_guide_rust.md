# Porting Guide: Python to Rust

This guide outlines the steps required to port LMUFFB from Python to Rust, prioritizing memory safety and modern tooling.

## 1. Development Environment

*   **Installer**: `rustup` (Windows).
*   **Toolchain**: `stable-x86_64-pc-windows-msvc`.
*   **IDE**: VS Code with `rust-analyzer` extension.

## 2. Dependencies (Crates)

Add these to your `Cargo.toml`:

```toml
[dependencies]
windows = { version = "0.52", features = ["Win32_System_Memory", "Win32_Foundation", "Win32_System_Threading"] }
libc = "0.2"
# Optional: 'vjoy' wrapper crate if available, or bindgen
```

## 3. Shared Memory Access

Rust uses the `windows` crate to call Win32 APIs. Accessing raw memory requires `unsafe` blocks.

**Snippet**:
```rust
use windows::Win32::System::Memory::{OpenFileMappingA, MapViewOfFile, FILE_MAP_READ};
use windows::Win32::Foundation::{CloseHandle, HANDLE};
use windows::core::PCSTR;
use std::ffi::CString;

#[repr(C)]
struct Rf2Telemetry {
    // Define fields matching C++ struct
    // Use types like f64, i32, [u8; 64]
    m_time: f64,
    // ...
}

fn main() -> windows::core::Result<()> {
    let map_name = CString::new("$rFactor2SMMP_Telemetry$").unwrap();
    
    unsafe {
        let handle = OpenFileMappingA(
            FILE_MAP_READ, 
            false, 
            PCSTR(map_name.as_ptr() as *const u8)
        )?;

        if handle.is_invalid() {
            panic!("Could not open file mapping");
        }

        let ptr = MapViewOfFile(handle, FILE_MAP_READ, 0, 0, std::mem::size_of::<Rf2Telemetry>());
        if ptr.is_null() {
            panic!("Could not map view of file");
        }

        let telemetry = &*(ptr as *const Rf2Telemetry);

        loop {
            // Read data safely (once cast)
            let rpm = telemetry.m_engine_rpm; // Hypothetical field
            
            // ... Logic ...
            
            std::thread::sleep(std::time::Duration::from_millis(2));
        }
        
        // Cleanup happens when handle is dropped if wrapped properly, 
        // but raw handles need CloseHandle(handle);
    }
    
    Ok(())
}
```

## 4. Data Structures

You must define the structs with `#[repr(C)]` to ensure they match the C layout in memory.

```rust
#[repr(C)]
pub struct Rf2Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[repr(C)]
pub struct Rf2Wheel {
    pub m_suspension_deflection: f64,
    // ... all other fields
}

#[repr(C)]
pub struct Rf2Telemetry {
    pub m_time: f64,
    // ...
    pub m_wheels: [Rf2Wheel; 4],
}
```

## 5. FFB Output (vJoy)

There are a few Rust crates for vJoy (e.g., `vjoy-rs`), but they might be unmaintained. The most robust method is to link against `vJoyInterface.dll` using `libloading` or `bindgen`.

**Using `libloading` (Dynamic Loading)**:
1.  Load `vJoyInterface.dll`.
2.  Get symbols for `AcquireVJD`, `SetAxis`, `RelinquishVJD`.
3.  Call them inside `unsafe` blocks.

## 6. FFB Engine Logic

Porting the Python logic to Rust is ideal for safety.

*   **Structs**: Create a `FfbEngine` struct holding state (smoothing buffers).
*   **Traits**: Implement traits like `Default` for initialization.
*   **Math**: Rust's `f64` methods (`.min()`, `.max()`, `.abs()`) map directly to Python's.

## 7. Performance Notes

*   Rust's release builds (`cargo build --release`) are comparable to C++ in speed.
*   **Safety**: Rust prevents buffer overflows when accessing arrays (like `m_wheels`), but since the raw pointer comes from Shared Memory, the initial dereference is `unsafe`. Once wrapped in a safe abstraction, the rest of the app is protected.
