use crate::{error::FailCode, types::Endpoint};

use caniot_sys as ll;

pub enum TelemetryError {
    Invalid = ll::caniot_error_t::CANIOT_EINVAL as isize,
    NotSupported = ll::caniot_error_t::CANIOT_ENOTSUP as isize,
    Frame = ll::caniot_error_t::CANIOT_EFRAME as isize,
}

impl From<TelemetryError> for FailCode {
    fn from(err: TelemetryError) -> Self {
        unsafe { FailCode::new_unchecked(err as u32) }
    }
}

pub enum AttributeError {
    NoSuchAttr = ll::caniot_error_t::CANIOT_ENOATTR as isize,
    ReadOnlyAttr = ll::caniot_error_t::CANIOT_EROATTR as isize,
    ReadAttrError = ll::caniot_error_t::CANIOT_EREADATTR as isize,
    WriteAttrError = ll::caniot_error_t::CANIOT_EWRITEATTR as isize,
}

impl From<AttributeError> for FailCode {
    fn from(err: AttributeError) -> Self {
        unsafe { FailCode::new_unchecked(err as u32) }
    }
}

pub trait DeviceApi {
    fn telemetry(&mut self, _ep: Endpoint) -> Result<Vec<u8>, TelemetryError> {
        Err(TelemetryError::NotSupported)
    }

    fn command(&mut self, _ep: Endpoint, _data: &[u8]) -> Result<(), TelemetryError> {
        Err(TelemetryError::NotSupported)
    }

    fn read_attribute(&mut self, _attr: u16) -> Result<u32, AttributeError> {
        Err(AttributeError::NoSuchAttr)
    }

    fn write_attribute(&mut self, _attr: u16, _value: u32) -> Result<(), AttributeError> {
        Err(AttributeError::NoSuchAttr)
    }
}
