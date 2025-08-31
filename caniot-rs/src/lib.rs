use thiserror::Error;

pub mod attributes;
pub mod controller;
pub mod datatypes;
pub mod device;
pub mod did;
pub mod driver;
pub mod error;
pub mod frame;
pub mod payload;
pub mod sys_control;
pub mod types;
pub mod utils;

#[cfg(test)]
mod attributes_test;

#[cfg(test)]
mod sys_control_test;

#[cfg(test)]
mod datatypes_test;

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
