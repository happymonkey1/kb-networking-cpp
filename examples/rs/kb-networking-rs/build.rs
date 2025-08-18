use std::env;
use std::path::PathBuf;
use std::process::Command;


fn main() {
    println!("VCPKG_ROOT = {}", env::var("VCPKG_ROOT").unwrap_or_else(|_| "Not Set".to_string()));
    println!("VCPKG_OVERLAY_PORTS = {}", env::var("VCPKG_OVERLAY_PORTS").unwrap_or_else(|_| "Not Set".to_string()));

    Command::new("vcpkg")
        .args(["search", "kbnetworking"])
        .output()
        .expect("vcpkg works");

    // vcpkg::find_package("kbnetworking").expect("Could not find kbnetworking via vcpkg");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(format!("{}/../../../build", manifest_dir.display()).to_string());
    let cpp_project_root = manifest_dir.join("../../../");

    let configure_status = Command::new("cmake")
        .arg("-S")
        .arg(&cpp_project_root)
        .arg("--preset=vcpkg".to_string())
        .arg(format!("-D CMAKE_INSTALL_PREFIX={}", out_dir.display()))
        .status()
        .expect("Failed to execute CMake configure step.");

    if !configure_status.success() {
        panic!("CMake configure step failed.");
    }

    let build_status = Command::new("cmake")
        .arg("--build")
        .arg(cpp_project_root.join("build"))
        .arg("--parallel")
        .status()
        .expect("Failed to execute CMake build step.");

    if !build_status.success() {
        panic!("CMake build step failed.");
    }

    // println!("cargo:rustc-link-search=native={}", out_dir.join("lib").display());
    println!("cargo:rustc-link-search={}/", out_dir.display());
    println!("cargo:rustc-link-search={}/vcpkg_installed/x64-linux/lib/", out_dir.display());
    println!("cargo:rustc-link-lib=static=kb-networking");
    println!("cargo:rustc-link-lib=static=fmt");
    println!("cargo:rustc-link-lib=static=GameNetworkingSockets_s");
    println!("cargo:rustc-link-lib=static=protobuf");

    println!("cargo:rerun-if-changed=../../../src/");
    println!("cargo:rerun-if-changed=../../../include/");
    println!("cargo:rerun-if-changed=../../../CMakeLists.txt");
    println!("cargo:rerun-if-changed=../../../CMakePresets.json");

    println!("cargo:rustc-link-lib=stdc++");

    let bindings = bindgen::Builder::default()
        .header("kb_networking_wrapper.h")
        .clang_arg("-I../../../include")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file(PathBuf::from(env::var("OUT_DIR").expect("out dir is set").to_string()).join("bindings.rs"))
        .expect("Couldn't write bindings!");
}