use caniot_sys as ll;

use core::{mem::MaybeUninit, ptr::NonNull};

use std::os::fd::{AsFd, AsRawFd};

use super::Driver;

pub struct LinuxDriver {
    data: ll::linux_api_context,
}

impl LinuxDriver {
    pub fn init(iface: impl AsRef<str>) -> Result<Self, i32> {
        let iface = iface.as_ref();
        let iface_cstr = std::ffi::CString::new(iface).unwrap();

        let mut data = MaybeUninit::<ll::linux_api_context>::uninit();
        let data = unsafe {
            let ret = ll::caniot_linux_api_ctx_init(data.as_mut_ptr(), iface_cstr.as_ptr(), 0u32);
            if ret < 0 {
                return Err(ret);
            }

            data.assume_init()
        };

        assert!(data.sock >= 0, "Socket initialization failed");

        Ok(Self { data })
    }
}

impl AsRawFd for LinuxDriver {
    fn as_raw_fd(&self) -> std::os::fd::RawFd {
        self.data.sock
    }
}

impl AsFd for LinuxDriver {
    fn as_fd(&self) -> std::os::fd::BorrowedFd<'_> {
        unsafe { std::os::fd::BorrowedFd::borrow_raw(self.data.sock) }
    }
}

impl Driver for LinuxDriver {
    fn get_api(&self) -> NonNull<ll::caniot_drivers_api> {
        unsafe { NonNull::new_unchecked(ll::linux_driver_api_ptr as *mut ll::caniot_drivers_api) }
    }

    fn get_data(&mut self) -> *mut ::core::ffi::c_void {
        &mut self.data as *mut _ as *mut _
    }
}
