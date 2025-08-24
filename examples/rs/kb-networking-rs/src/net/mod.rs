use kb_networking_rs::kb_networking_cpp::bindings;

pub mod server;

pub serializer_t ConnectionHandle = bindings::kb_conn_t;

pub fn kb_start_server() {

}