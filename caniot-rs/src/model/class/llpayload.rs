use std::mem::MaybeUninit;

use crate::{class::TempSensType, datatypes::{Temperature, Xps}, error::FailCode};

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

pub trait LLTelemetry: LLPayload {
    type IOType;

    const GET_TEMP_FN: unsafe extern "C" fn(
        t: *const Self,
        sensor: ll::caniot_temp_sens_t::Type,
        temperature: *mut u16,
    ) -> ::core::ffi::c_int;

    const SET_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: ll::caniot_temp_sens_t::Type,
        temperature: u16,
    ) -> ::core::ffi::c_int;

    const CLEAR_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: ll::caniot_temp_sens_t::Type,
    ) -> ::core::ffi::c_int;

    const GET_IO_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        state: *mut bool,
    ) -> ::core::ffi::c_int;

    const SET_IO_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        state: bool,
    ) -> ::core::ffi::c_int;

    fn set_temperature(&mut self, sensor: TempSensType, celsius: f32) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let temperature = Temperature::from_celsius(celsius);

        let ret = unsafe {
            (Self::SET_TEMP_FN)(self, sensor, temperature.to_raw_u10())
        };
        FailCode::to_errno(ret)
    }

    fn clear_temperature(&mut self, sensor: TempSensType) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let ret = unsafe { (Self::CLEAR_TEMP_FN)(self, sensor) };
        FailCode::to_errno(ret)
    }

    fn get_temperature(&self, sensor: TempSensType) -> Option<f32> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor).ok()?;
        let mut temperature: u16 = 0;
        let ret =
            unsafe { (Self::GET_TEMP_FN)(self, sensor, &mut temperature) };
        FailCode::to_errno(ret).ok()?;
        Temperature::from_raw_u10(temperature).to_celsius()
    }

    fn set_io(&mut self, io: impl TryInto<Self::IOType, Error = FailCode>, state: bool) -> Result<(), FailCode> {
        let io = io.try_into()?;
        let ret = unsafe { (Self::SET_IO_FN)(self, io, state) };
        FailCode::to_errno(ret)
    }

    fn get_io(&self, io: impl TryInto<Self::IOType, Error = FailCode>) -> Option<bool> {
        let io = io.try_into().ok()?;
        let mut state = false;
        let ret = unsafe { (Self::GET_IO_FN)(self, io, &mut state) };
        FailCode::to_result(ret).ok()?;
        Some(state)
    }
}

pub trait LLCommand: LLPayload {
    type IOType;

    const GET_IO_XPS_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        xps: *mut ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int;

    const SET_IO_XPS_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        xps: ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int;

    fn set_io_xps(&mut self, io: impl TryInto<Self::IOType, Error = FailCode>, xps: Xps) -> Result<(), FailCode> {
        let io = io.try_into()?;
        let xps = xps.into();
        let ret = unsafe { (Self::SET_IO_XPS_FN)(self, io, xps) };
        FailCode::to_errno(ret)
    }

    fn get_io_xps(&self, io: impl TryInto<Self::IOType, Error = FailCode>) -> Result<Xps, FailCode> {
        let io = io.try_into()?;
        let mut xps = ll::caniot_complex_digital_cmd_t::CANIOT_XPS_NONE;
        let ret = unsafe { (Self::GET_IO_XPS_FN)(self, io, &mut xps) };
        FailCode::to_errno(ret)?;
        Ok(Xps::from(xps))
    }
}

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
