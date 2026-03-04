pub mod class0;
pub mod class1;

pub mod llpayload;

use caniot_sys as ll;
use serde::{Deserialize, Serialize};

use crate::{Temperature, error::FailCode};

#[cfg(test)]
mod class0_test;

#[cfg(test)]
mod class1_test;

#[derive(Debug, Clone, Copy)]
pub enum TempSensType {
    BoardSensor,
    ExternalSensor(u8),
    AvgExternal,
    AnyExternal,
    Any,
}

impl TryFrom<ll::caniot_temp_sens_t::Type> for TempSensType {
    type Error = FailCode;

    fn try_from(value: ll::caniot_temp_sens_t::Type) -> Result<Self, Self::Error> {
        match value {
            ll::caniot_temp_sens_t::CANIOT_TEMP_INT => Ok(TempSensType::BoardSensor),
            ll::caniot_temp_sens_t::CANIOT_TEMP_EXT1 => Ok(TempSensType::ExternalSensor(0)),
            ll::caniot_temp_sens_t::CANIOT_TEMP_EXT2 => Ok(TempSensType::ExternalSensor(1)),
            ll::caniot_temp_sens_t::CANIOT_TEMP_EXT3 => Ok(TempSensType::ExternalSensor(2)),
            _ => Err(FailCode::EINVAL),
        }
    }
}

impl TryFrom<TempSensType> for ll::caniot_temp_sens_t::Type {
    type Error = FailCode;

    fn try_from(value: TempSensType) -> Result<Self, Self::Error> {
        match value {
            TempSensType::BoardSensor => Ok(ll::caniot_temp_sens_t::CANIOT_TEMP_INT),
            TempSensType::ExternalSensor(0) => Ok(ll::caniot_temp_sens_t::CANIOT_TEMP_EXT1),
            TempSensType::ExternalSensor(1) => Ok(ll::caniot_temp_sens_t::CANIOT_TEMP_EXT2),
            TempSensType::ExternalSensor(2) => Ok(ll::caniot_temp_sens_t::CANIOT_TEMP_EXT3),
            _ => Err(FailCode::EINVAL),
        }
    }
}

pub trait TelemetryTrait {
    fn get_temperature(&self, sensor: TempSensType) -> Option<Temperature>;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum BoardClassTelemetry {
    Class0(class0::TelemetryData),
    Class1(class1::TelemetryData),
}

impl BoardClassTelemetry {
    pub fn class_id(&self) -> u8 {
        match self {
            BoardClassTelemetry::Class0(_) => 0,
            BoardClassTelemetry::Class1(_) => 1,
        }
    }

    pub fn as_class0(&self) -> Option<&class0::TelemetryData> {
        match self {
            BoardClassTelemetry::Class0(telemetry) => Some(telemetry),
            _ => None,
        }
    }

    pub fn as_class1(&self) -> Option<&class1::TelemetryData> {
        match self {
            BoardClassTelemetry::Class1(telemetry) => Some(telemetry),
            _ => None,
        }
    }

    pub fn get_temperature(&self, sensor: TempSensType) -> Option<Temperature> {
        match self {
            BoardClassTelemetry::Class0(telemetry) => telemetry.get_temperature(sensor),
            BoardClassTelemetry::Class1(telemetry) => telemetry.get_temperature(sensor),
        }
    }
}
