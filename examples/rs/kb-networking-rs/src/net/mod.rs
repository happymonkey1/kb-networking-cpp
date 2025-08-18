use kb_networking_rs::kb_networking_cpp::bindings;

pub mod server;

pub type ConnectionHandle = bindings::kb_conn_t;

pub fn kb_start_server() {

}