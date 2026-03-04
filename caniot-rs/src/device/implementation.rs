use crate::{error::FailCode, payload::Payload, types::Endpoint};

pub trait DeviceApi {
    fn telemetry(&mut self, _ep: Endpoint) -> Result<Payload, FailCode> {
        Err(FailCode::ENOTSUP)
    }

    fn command(&mut self, _ep: Endpoint, _data: &[u8]) -> Result<(), FailCode> {
        Err(FailCode::ENOTSUP)
    }

    fn read_attribute(&mut self, _attr: u16) -> Result<u32, FailCode> {
        Err(FailCode::ENOATTR)
    }

    fn write_attribute(&mut self, _attr: u16, _value: u32) -> Result<(), FailCode> {
        Err(FailCode::ENOATTR)
    }
}
