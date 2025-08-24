use crate::kb_networking_cpp;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum LogLevel {
    None = 0,
    Trace = 1,
    Debug = 2,
    Info = 3,
    Warn = 4,
    Error = 5,
    Critical = 6,
}

// The main wrapper macro
#[macro_export]
macro_rules! kb_log {
    // The macro takes a LogLevel and then format arguments, just like println!
    ($level:expr, $($arg:tt)*) => {
        {
            // 1. Use Rust's `format!` to create the complete string.
            let formatted_str = format!($($arg)*);

            // 2. Convert the Rust String to a C-compatible, null-terminated string.
            //    This will panic if the string contains an reserved null byte.
            let c_msg = std::ffi::CString::new(formatted_str)
                .expect("CString::new failed: string contained null byte");

            // 3. Call the C function within an unsafe block.
            unsafe {
                // We cast our Rust enum to the bindgen-generated serializer_t.
                // bindgen will likely represent the C enum as a u32 or similar.
                let c_level = $level as kb_networking_cpp::bindings::kb_log_level;

                // Call the C function with the level and the pointer to the C string.
                kb_networking_cpp::bindings::kb_log_internal(c_level, c_msg.as_ptr());
            }
        }
    };
}
