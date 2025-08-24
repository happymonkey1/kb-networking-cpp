use std::ffi::c_void;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::slice;
use kb_networking_rs::kb_networking_cpp::bindings;
use crate::net::ConnectionHandle;

struct Callbacks {
    on_connect: Box<dyn FnMut(ConnectionHandle) + Send + 'static>,
    on_disconnect: Box<dyn FnMut(ConnectionHandle) + Send + 'static>,
    on_data: Box<dyn FnMut(ConnectionHandle, &[u8]) + Send + 'static>,
}

pub struct UdpServer {
    ptr: *mut bindings::kb_server_t,
    callbacks: Option<Box<Callbacks>>,
}

impl UdpServer {
    pub fn new() -> Option<Self> {
        let server_ptr = unsafe { bindings::kb_server_create() };
        if server_ptr.is_null() {
            None
        } else {
            Some(UdpServer {
                ptr: server_ptr,
                callbacks: None,
            })
        }
    }

    /// Starts the server asynchronously on the given port, using the provided closures for events.
    pub fn start_async<FConn, FDisc, FData>(
        &mut self,
        port: u16,
        on_connect: FConn,
        on_disconnect: FDisc,
        on_data: FData,
    ) -> Result<(), ()>
    where
        FConn: FnMut(ConnectionHandle) + Send + 'static,
        FDisc: FnMut(ConnectionHandle) + Send + 'static,
        FData: FnMut(ConnectionHandle, &[u8]) + Send + 'static,
    {
        // Box up the closures into our Callbacks struct.
        let mut callbacks = Box::new(Callbacks {
            on_connect: Box::new(on_connect),
            on_disconnect: Box::new(on_disconnect),
            on_data: Box::new(on_data),
        });

        // Get a raw pointer to the heap-allocated callbacks. This is our `void*`.
        let user_data_ptr = callbacks.as_mut() as *mut Callbacks as *mut c_void;

        // Store the Box in the UdpServer struct to manage its lifetime.
        self.callbacks = Some(callbacks);

        unsafe {
            // Pass the C library our trampoline functions and the pointer to our callbacks.
            bindings::kb_server_set_callbacks(
                self.ptr,
                Some(on_data_trampoline),
                Some(on_connect_trampoline),
                Some(on_disconnect_trampoline),
                user_data_ptr,
            );

            // Start the server.
            if bindings::kb_server_start_async(self.ptr, port) {
                Ok(())
            } else {
                Err(())
            }
        }
    }

    /// Stops the server.
    pub fn stop(&mut self) {
        unsafe { bindings::kb_server_stop(self.ptr) }
    }

    /// Sends a packet of data to a specific client.
    pub fn send(&mut self, conn: ConnectionHandle, data: &[u8], reliable: bool) -> bool {
        unsafe {
            bindings::kb_server_send(
                self.ptr,
                conn,
                data.as_ptr() as *const c_void,
                data.len() as u32,
                reliable,
            )
        }
    }

    /// Broadcasts a packet of data to all connected clients.
    pub fn broadcast(&mut self, data: &[u8], reliable: bool) {
        unsafe {
            bindings::kb_server_broadcast(
                self.ptr,
                data.as_ptr() as *const c_void,
                data.len() as u32,
                reliable,
            )
        }
    }

    /// Disconnects a client.
    pub fn disconnect(&mut self, conn: ConnectionHandle, reason_code: i32) {
        unsafe { bindings::kb_server_disconnect(self.ptr, conn, reason_code) }
    }

    /// Checks if the server is currently running.
    pub fn is_running(&self) -> bool {
        unsafe { bindings::kb_server_is_running(self.ptr) }
    }

    /// Returns the port the server is listening on.
    pub fn port(&self) -> u16 {
        unsafe { bindings::kb_server_port(self.ptr) }
    }
}

impl Drop for UdpServer {
    fn drop(&mut self) {
        if self.is_running() {
            self.stop();
        }

        unsafe { bindings::kb_server_destroy(self.ptr) }
    }
}

extern "C" fn on_connect_trampoline(
    _p_server: *mut bindings::kb_server,
    p_conn: bindings::kb_conn_t,
    p_user_data: *mut c_void,
) {
    if p_user_data.is_null() { return; }
    // Safely execute the closure, catching any panics.
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let callbacks = unsafe { &mut *(p_user_data as *mut Callbacks) };
        (callbacks.on_connect)(p_conn);
    }));
}

extern "C" fn on_disconnect_trampoline(
    _p_server: *mut bindings::kb_server,
    p_conn: bindings::kb_conn_t,
    p_user_data: *mut c_void,
) {
    if p_user_data.is_null() { return; }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let callbacks = unsafe { &mut *(p_user_data as *mut Callbacks) };
        (callbacks.on_disconnect)(p_conn);
    }));
}

extern "C" fn on_data_trampoline(
    _p_server: *mut bindings::kb_server,
    p_conn: bindings::kb_conn_t,
    p_data: *const c_void,
    p_len: u32,
    p_user_data: *mut c_void,
) {
    if p_user_data.is_null() { return; }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let callbacks = unsafe { &mut *(p_user_data as *mut Callbacks) };
        // Convert the C buffer into a safe Rust slice. This is a zero-copy_steam_to_ip_address operation.
        let data_slice = unsafe { slice::from_raw_parts(p_data as *const u8, p_len as usize) };
        (callbacks.on_data)(p_conn, data_slice);
    }));
}

#[cfg(test)]
mod tests {
    use crate::net::server::UdpServer;

    #[test]
    fn when_create_server_then_succeed() {
        let server = UdpServer::new();
        assert!(server.is_some())
    }

}