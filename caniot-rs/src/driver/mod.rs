pub mod dummy;
#[cfg(feature = "std")]
pub mod linux;

pub use dummy::DummyDriver;
#[cfg(feature = "std")]
pub use linux::LinuxDriver;

use std::ptr::NonNull;
use caniot_sys as ll;

pub trait Driver {
    fn get_api(&self) -> NonNull<ll::caniot_drivers_api>;

    fn get_data(&mut self) -> *mut ::core::ffi::c_void;
}
