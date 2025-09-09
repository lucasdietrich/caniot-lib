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

impl Into<Vec<u8>> for HeatingControllerCommand {
    fn into(self) -> Vec<u8> {
        let mut payload = Vec::with_capacity(2);

        payload.push(self.modes[0] as u8 | (self.modes[1] as u8) << 4);
        payload.push(self.modes[2] as u8 | (self.modes[3] as u8) << 4);

        payload
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

impl Into<Vec<u8>> for HeatingControllerTelemetry {
    fn into(self) -> Vec<u8> {
        let mut payload = Vec::with_capacity(3);

        payload.push(self.modes[0] as u8 | (self.modes[1] as u8) << 4);
        payload.push(self.modes[2] as u8 | (self.modes[3] as u8) << 4);
        payload.push(self.power_status as u8);
        payload
    }
}
