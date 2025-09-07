use std::mem::MaybeUninit;

use crate::error::FailCode;

use caniot_sys as ll;

pub trait HasEffect: LLPayload {
    fn has_effect(&self) -> bool {
        self != &Self::default()
    }
}

pub trait LLPayload: Sized + PartialEq + Eq + Clone {
    const SER_SIZE: u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int;

    fn serialize(&self) -> Result<Vec<u8>, FailCode> {
        let mut len = Self::SER_SIZE;
        let mut buf = Vec::with_capacity(len as usize);
        let buf_ptr = buf.as_mut_ptr();
        let ret = unsafe { (Self::SER_FN)(self, buf_ptr, &mut len) };
        unsafe {
            buf.set_len(len as usize);
        }
        FailCode::to_result(ret)?;
        Ok(buf)
    }

    fn deserialize(data: &[u8]) -> Result<Self, FailCode> {
        if data.len() != Self::SER_SIZE as usize {
            return Err(unsafe { FailCode::new_unchecked(ll::caniot_error_t::CANIOT_EINVAL) });
        }

        let mut buffer: MaybeUninit<Self> = MaybeUninit::uninit();
        let ret = unsafe { (Self::DESER_FN)(buffer.as_mut_ptr(), data.as_ptr(), data.len() as u8) };
        FailCode::to_result(ret)?;
        Ok(unsafe { buffer.assume_init() })
    }

    fn default() -> Self {
        let mut buffer: MaybeUninit<Self> = MaybeUninit::uninit();
        let ret = unsafe { (Self::DEFAULT_FN)(buffer.as_mut_ptr()) };
        FailCode::to_result(ret).expect("Failed to set default telemetry");
        unsafe { buffer.assume_init() }
    }
}

impl LLPayload for ll::caniot_blc0_telemetry {
    const SER_SIZE: u8 = ll::CANIOT_BLC0_TELEMETRY_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc0_telemetry_defaults;
}

impl LLPayload for ll::caniot_blc0_command {
    const SER_SIZE: u8 = ll::CANIOT_BLC0_COMMAND_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc0_command_defaults;
}

impl HasEffect for ll::caniot_blc0_command {}

impl LLPayload for ll::caniot_blc1_telemetry {
    const SER_SIZE: u8 = ll::CANIOT_BLC1_TELEMETRY_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc1_telemetry_defaults;
}

impl LLPayload for ll::caniot_blc1_command {
    const SER_SIZE: u8 = ll::CANIOT_BLC1_COMMAND_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc1_command_defaults;
}

impl HasEffect for ll::caniot_blc1_command {}

#[cfg(test)]
mod tests {
    use std::fmt::Debug;

    use super::*;

    #[test]
    fn test_ser_all() {
        fn test_one<L: LLPayload + Debug>() {
            let default = L::default();
            let serialized = default.serialize().unwrap();
            assert_eq!(serialized.len(), L::SER_SIZE as usize);
            let deserialized = L::deserialize(&serialized).unwrap();
            assert_eq!(deserialized, default);
        }

        test_one::<ll::caniot_blc0_telemetry>();
        test_one::<ll::caniot_blc0_command>();
        test_one::<ll::caniot_blc1_telemetry>();
        test_one::<ll::caniot_blc1_command>();
    }

    #[test]
    fn test_has_effect() {
        fn test_one<L: HasEffect + Debug>() {
            let instance = L::default();
            assert_eq!(instance.has_effect(), instance != L::default());
        }

        test_one::<ll::caniot_blc0_command>();
        test_one::<ll::caniot_blc1_command>();
    }
}
