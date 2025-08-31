use caniot_sys as ll;

use core::ptr::NonNull;

use crate::{device::implementation::DeviceApi, error::FailCode, types::Endpoint};

pub(super) struct DeviceApiWrapper {
    callbacks: Box<ll::caniot_device_api>, // addr shouldn't move, device has pointers which reference api callbacks
    data: Box<Box<dyn DeviceApi>>, // Box<dyn ..> is a fat pointer, so wrap it again in a Box
}

fn get_api_from_dev(dev: *mut ll::caniot_device) -> &'static mut dyn DeviceApi {
    assert!(dev != core::ptr::null_mut());
    let api_data_ptr = unsafe { (*dev).api_data };
    let api_data = unsafe { &mut *(api_data_ptr as *mut Box<dyn DeviceApi>) };
    let api = api_data.as_mut();
    api
}

unsafe extern "C" fn trampoline_telemetry_cb(
    dev: *mut ll::caniot_device,
    ep: ll::caniot_endpoint_t::Type,
    buf: *mut ::core::ffi::c_uchar,
    len: *mut u8,
) -> ::core::ffi::c_int {
    assert!(buf != core::ptr::null_mut());

    let api = get_api_from_dev(dev);
    let endpoint = Endpoint::try_from(ep).unwrap();

    match api.telemetry(endpoint) {
        Ok(data) => {
            let data_len = data.len();
            if !buf.is_null() && !len.is_null() && data_len <= 8 {
                unsafe {
                    core::ptr::copy_nonoverlapping(data.as_ptr(), buf, data_len);
                    *len = data_len as u8;
                }
                0
            } else {
                ll::caniot_error_t::CANIOT_EINVAL as i32 // Invalid arguments
            }
        }
        Err(err) => {
            let code: FailCode = err.into();
            code.into()
        }
    }
}

unsafe extern "C" fn trampoline_command_cb(
    dev: *mut ll::caniot_device,
    ep: ll::caniot_endpoint_t::Type,
    buf: *const ::core::ffi::c_uchar,
    len: u8,
) -> ::core::ffi::c_int {
    assert!(buf != core::ptr::null_mut());
    assert!(len as usize <= 8);

    let api = get_api_from_dev(dev);
    let endpoint = Endpoint::try_from(ep).unwrap();
    let data = unsafe { core::slice::from_raw_parts(buf, len as usize) };

    match api.command(endpoint, data) {
        Ok(()) => 0,
        Err(err) => {
            let code: FailCode = err.into();
            code.into()
        }
    }
}

unsafe extern "C" fn trampoline_read_attribute_cb(
    dev: *mut ll::caniot_device,
    key: u16,
    val: *mut u32,
) -> ::core::ffi::c_int {
    assert!(val != core::ptr::null_mut());

    let api = get_api_from_dev(dev);

    match api.read_attribute(key) {
        Ok(data) => {
            unsafe {
                *val = data;
            }
            0
        }
        Err(err) => {
            let code: FailCode = err.into();
            code.into()
        }
    }
}

unsafe extern "C" fn trampoline_write_attribute_cb(
    dev: *mut ll::caniot_device,
    key: u16,
    val: u32,
) -> ::core::ffi::c_int {
    let api = get_api_from_dev(dev);

    match api.write_attribute(key, val) {
        Ok(()) => 0,
        Err(err) => {
            let code: FailCode = err.into();
            code.into()
        }
    }
}

unsafe extern "C" fn trampoline_blc_sys_cmd_cb(
    dev: *mut ll::caniot_device,
    sys_cmd: ll::caniot_blc_sys_cmd_t::Type,
) -> ::core::ffi::c_int {
    let api = get_api_from_dev(dev);

    todo!()
}

impl DeviceApiWrapper {
    pub fn new<A: DeviceApi + 'static>(api: A) -> DeviceApiWrapper {
        let mut callbacks: ll::caniot_device_api = unsafe { core::mem::zeroed() };
        callbacks.telemetry_handler = Some(trampoline_telemetry_cb);
        callbacks.command_handler = Some(trampoline_command_cb);
        callbacks.custom_attr.read = Some(trampoline_read_attribute_cb);
        callbacks.custom_attr.write = Some(trampoline_write_attribute_cb);
        // callbacks.blc_sys_cmd_handler = Some(trampoline_blc_sys_cmd_cb);

        let api = Box::new(api);

        DeviceApiWrapper {
            callbacks: Box::new(callbacks),
            data: Box::new(api),
        }
    }

    pub fn get_api(&self) -> NonNull<ll::caniot_device_api> {
        unsafe {
            NonNull::new_unchecked(self.callbacks.as_ref() as *const ll::caniot_device_api
                as *mut ll::caniot_device_api)
        }
    }

    pub fn get_data(&mut self) -> *mut ::core::ffi::c_void {
        self.data.as_mut() as *mut Box<dyn DeviceApi> as *mut ::core::ffi::c_void
    }
}
