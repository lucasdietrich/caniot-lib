use std::ptr::NonNull;

use caniot_sys as ll;

use super::Driver;

pub struct DummyDriver;

impl Driver for DummyDriver {
    fn get_api(&self) -> NonNull<ll::caniot_drivers_api> {
        unsafe { NonNull::new_unchecked(ll::dummy_driver_api_ptr as *mut ll::caniot_drivers_api) }
    }

    fn get_data(&mut self) -> *mut ::core::ffi::c_void {
        core::ptr::null_mut()
    }
}
