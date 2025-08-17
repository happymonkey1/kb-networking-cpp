#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod kb_networking_cpp {
    // `include!` macro dumps the bindings into the crate's main entry point
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
