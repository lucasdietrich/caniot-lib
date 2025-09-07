pub mod class0;
pub mod class1;

pub mod llpayload;

use caniot_sys as ll;

use crate::error::FailCode;

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
    fn get_temperature(&self, sensor: TempSensType) -> Option<f32>;
}
