pub mod attributes;
pub mod class;
pub mod datatypes;
pub mod did;
pub mod error;
pub mod math;
pub mod sys_control;
pub mod types;

pub use attributes::Attribute;
pub use class::{class0, class1, TempSensType, TelemetryTrait, BoardClassTelemetry};
pub use datatypes::*;
pub use did::DeviceId;
pub use error::FailCode;
pub use sys_control::SysCtrl;
pub use types::{Action, Direction, Endpoint, Type};

#[cfg(test)]
mod sys_control_test;

#[cfg(test)]
mod attributes_test;
#[cfg(test)]
mod datatypes_test;
