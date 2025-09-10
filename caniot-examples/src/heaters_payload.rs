use caniot::{datatypes::HeatingMode, error::FailCode};
use num::FromPrimitive;

#[derive(Debug, Default, Clone, Copy, PartialEq)]
pub struct HeatingControllerCommand {
    pub modes: [HeatingMode; 4],
}

impl TryFrom<&[u8]> for HeatingControllerCommand {
    type Error = FailCode;

    fn try_from(payload: &[u8]) -> Result<Self, FailCode> {
        if payload.len() >= 2 {
            Ok(HeatingControllerCommand {
                modes: [
                    HeatingMode::from_u8(payload[0] & 0xf).unwrap(),
                    HeatingMode::from_u8((payload[0] & 0xf0) >> 4).unwrap(),
                    HeatingMode::from_u8(payload[1] & 0xf).unwrap(),
                    HeatingMode::from_u8((payload[1] & 0xf0) >> 4).unwrap(),
                ],
            })
        } else {
            Err(FailCode::EFRAME)
        }
    }
}

impl From<HeatingControllerCommand> for Vec<u8> {
    fn from(val: HeatingControllerCommand) -> Self {
        vec![
            val.modes[0] as u8 | (val.modes[1] as u8) << 4,
            val.modes[2] as u8 | (val.modes[3] as u8) << 4,
        ]
    }
}

#[derive(Debug, Default, Clone, Copy, PartialEq)]
pub struct HeatingControllerTelemetry {
    pub modes: [HeatingMode; 4],
    pub power_status: bool,
}

impl TryFrom<&[u8]> for HeatingControllerTelemetry {
    type Error = FailCode;

    fn try_from(payload: &[u8]) -> Result<Self, FailCode> {
        if payload.len() >= 3 {
            Ok(HeatingControllerTelemetry {
                modes: [
                    HeatingMode::from_u8(payload[0] & 0xf).unwrap(),
                    HeatingMode::from_u8((payload[0] & 0xf0) >> 4).unwrap(),
                    HeatingMode::from_u8(payload[1] & 0xf).unwrap(),
                    HeatingMode::from_u8((payload[1] & 0xf0) >> 4).unwrap(),
                ],
                power_status: payload[2] & 0b0000_0001 != 0,
            })
        } else {
            Err(FailCode::EFRAME)
        }
    }
}

impl From<HeatingControllerTelemetry> for Vec<u8> {
    fn from(val: HeatingControllerTelemetry) -> Self {
        vec![
            val.modes[0] as u8 | (val.modes[1] as u8) << 4,
            val.modes[2] as u8 | (val.modes[3] as u8) << 4,
            val.power_status as u8,
        ]
    }
}
