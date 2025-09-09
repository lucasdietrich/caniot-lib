use core::{fmt::Debug, fmt::Display, num::NonZeroU32};

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct FailCode {
    code: NonZeroU32,
}

impl FailCode {
    pub const EINVAL: Self = unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_EINVAL) };
    pub const EAGAIN: Self = unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_EAGAIN) };
    pub const ENOTSUP: Self =
        unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_ENOTSUP) };
    pub const EFRAME: Self = unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_EFRAME) };
    pub const ENOATTR: Self =
        unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_ENOATTR) };

    pub(crate) const unsafe fn new_unchecked(code: u32) -> Self {
        FailCode {
            code: unsafe { NonZeroU32::new_unchecked(code) },
        }
    }

    pub fn to_result(ret: i32) -> Result<u32, FailCode> {
        match ret {
            code if code >= 0 => Ok(code as u32),
            code => Err(unsafe { FailCode::new_unchecked((-code) as u32) }),
        }
    }

    pub fn to_errno(ret: i32) -> Result<(), FailCode> {
        Self::to_result(ret).map(|_| ())
    }

    pub fn is_eagain(&self) -> bool {
        self.code.get() == ll::caniot_error_t::CANIOT_EAGAIN
    }
}

impl Debug for FailCode {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "FailCode({:x})", self.code.get())
    }
}

impl Into<::core::ffi::c_int> for FailCode {
    fn into(self) -> ::core::ffi::c_int {
        self.code.get() as ::core::ffi::c_int
    }
}

use caniot_sys as ll;

impl Display for FailCode {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let code = self.code.get();
        let err_str = unsafe { ll::caniot_error_to_string(code) };
        let err_cstr = unsafe { core::ffi::CStr::from_ptr(err_str) };

        write!(
            f,
            "Caniot Error (0x{:04X}): {}",
            code,
            err_cstr.to_string_lossy()
        )
    }
}

impl core::error::Error for FailCode {}

#[cfg(test)]
mod tests {
    use super::*;
    use caniot_sys as ll;
}
