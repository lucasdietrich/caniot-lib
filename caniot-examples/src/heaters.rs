use caniot::{
    datatypes::HeatingMode, device::implementation::DeviceApi, error::FailCode, payload::Payload, types::Endpoint
};

use crate::{NodeApi, heaters_payload::HeatingControllerCommand};

pub struct HeatersController {
    modes: [HeatingMode; 4],
    power_status: bool,
}

impl NodeApi for HeatersController {}

impl Default for HeatersController {
    fn default() -> Self {
        Self {
            modes: [
                HeatingMode::Stop,
                HeatingMode::Stop,
                HeatingMode::Stop,
                HeatingMode::Stop,
            ],
            power_status: true,
        }
    }
}

impl DeviceApi for HeatersController {
    fn command(&mut self, ep: Endpoint, data: &[u8]) -> Result<(), FailCode> {
        if !matches!(ep, Endpoint::ApplicationDefault) {
            return Err(FailCode::ENOTSUP);
        }

        let command = HeatingControllerCommand::try_from(data).map_err(|_| FailCode::EFRAME)?;

        for (i, mode) in command.modes.iter().enumerate() {
            if mode != &HeatingMode::None {
                self.modes[i] = *mode;
            }
        }

        Ok(())
    }

    fn telemetry(&mut self, _ep: Endpoint) -> Result<Payload, FailCode> {
        if !matches!(_ep, Endpoint::ApplicationDefault) {
            return Err(FailCode::ENOTSUP);
        }

        let telemetry = crate::heaters_payload::HeatingControllerTelemetry {
            modes: self.modes,
            power_status: self.power_status,
        };
        Ok(telemetry.into())
    }
}
