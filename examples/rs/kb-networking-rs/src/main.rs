pub use kb_networking_rs::kb_networking_cpp;
use crate::core::LogLevel;

mod core;
mod net;

fn main() {
    unsafe { kb_networking_cpp::bindings::kb_networking_init(); }

    kb_log!(LogLevel::Info, "Hello world!");
}
