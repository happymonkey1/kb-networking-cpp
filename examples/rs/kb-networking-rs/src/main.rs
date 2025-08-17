use kb_networking_rs::kb_networking_cpp;

fn main() {
    println!("Hello, world!");

    unsafe { kb_networking_cpp::kb_networking_init(); }
}
