use crate::{driver::Driver, error::FailCode, types::Endpoint};

use caniot_sys as ll;

pub enum ApiError {
    Invalid = ll::caniot_error_t::CANIOT_EINVAL as isize,
    Timeout = ll::caniot_error_t::CANIOT_ETIMEOUT as isize,
    Again = ll::caniot_error_t::CANIOT_EAGAIN as isize,
    NoSuchAttr = ll::caniot_error_t::CANIOT_ENOATTR as isize,
    ReadOnlyAttr = ll::caniot_error_t::CANIOT_EROATTR as isize,
    ReadAttrError = ll::caniot_error_t::CANIOT_EREADATTR as isize,
    WriteAttrError = ll::caniot_error_t::CANIOT_EWRITEATTR as isize,
    NotSupported = ll::caniot_error_t::CANIOT_ENOTSUP as isize,
    NotImplemented = ll::caniot_error_t::CANIOT_ENIMPL as isize,
}

impl From<ApiError> for FailCode {
    fn from(err: ApiError) -> Self {
        unsafe { FailCode::new_unchecked(err as u32) }
    }
}

pub trait DeviceApi {
    fn telemetry(&mut self, _ep: Endpoint) -> Result<Vec<u8>, ApiError> {
        Err(ApiError::NotSupported)
    }

    fn command(&mut self, _ep: Endpoint, _data: &[u8]) -> Result<(), ApiError> {
        Err(ApiError::NotSupported)
    }

    fn read_attribute(&mut self, _attr: u16) -> Result<u32, ApiError> {
        Err(ApiError::NotSupported)
    }

    fn write_attribute(&mut self, _attr: u16, _value: u32) -> Result<(), ApiError> {
        Err(ApiError::NotSupported)
    }
}
