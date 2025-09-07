use thiserror::Error;

pub mod controller;
pub mod device;
pub mod driver;
pub mod frame;
pub mod model;
pub mod utils;

pub use driver::*;
pub use model::*;

#[derive(Error, Debug)]
pub enum ProtocolError {
    #[error("Invalid device id")]
    DeviceIdCreationError,
    #[error("Payload decode error")]
    PayloadDecodeError,
    #[error("Payload encode error")]
    PayloadEncodeError,
    #[error("Command format error")]
    CommandEncodeError,
    #[error("Unknown attribute key")]
    UnknownAttributeKey,
    #[error("Invalid buffer size")]
    BufferSizeError,
    #[error("Invalid class payload size")]
    ClassPayloadSizeError,
    #[error("Invalid class command size")]
    ClassCommandSizeError,
    #[error("Unsupported caniot class")]
    UnsupportedClass,
}
