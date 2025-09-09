use std::time::Duration;

use caniot::{
    class::{
        class0::{self, IO}, llpayload::{LLCommand, LLPayload, LLTelemetry}, TempSensType
    },
    datatypes::{Temperature, Xps},
    device::implementation::DeviceApi,
    error::FailCode,
    types::Endpoint,
};
use expirable::Expirable;

use crate::{NodeApi, helpers::EmuXps};

const LIGHTS_PULSE_DURATION: Duration = Duration::from_secs(30);
const SIREN_PULSE_DURATION: Duration = Duration::from_secs(20);

#[derive(Default)]
pub struct OutdoorAlarmController {
    lights: [EmuXps; 2],         // oc1, oc2 (south, east)
    siren: EmuXps,               // rl1
    presence_sensors: [bool; 2], // in1, in2 (south, east)
    sabotage: bool,              // in4
}

impl OutdoorAlarmController {
    pub fn new() -> Self {
        Self {
            lights: [
                EmuXps::new(false, false, Some(LIGHTS_PULSE_DURATION)),
                EmuXps::new(false, false, Some(LIGHTS_PULSE_DURATION)),
            ],
            siren: EmuXps::new(false, false, Some(SIREN_PULSE_DURATION)),
            presence_sensors: [false, false],
            sabotage: false,
        }
    }
}

impl DeviceApi for OutdoorAlarmController {
    fn telemetry(&mut self, ep: Endpoint) -> Result<Vec<u8>, FailCode> {
        if ep != Endpoint::BoardControl {
            return Err(FailCode::ENOTSUP);
        }

        let mut telemetry = class0::Telemetry::default();

        telemetry.set_io(IO::Input1, self.presence_sensors[0])?;
        telemetry.set_io(IO::Input2, self.presence_sensors[1])?;
        telemetry.set_io(IO::Input4, self.sabotage)?;
        telemetry.set_io(IO::Oc1, self.lights[0].get_state())?;
        telemetry.set_io(IO::Oc1PulseActive, self.lights[0].pulse_pending())?;
        telemetry.set_io(IO::Oc2, self.lights[1].get_state())?;
        telemetry.set_io(IO::Oc2PulseActive, self.lights[1].pulse_pending())?;
        telemetry.set_io(IO::Relay1, self.siren.get_state())?;
        telemetry.set_io(IO::Relay1PulseActive, self.siren.pulse_pending())?;
        telemetry.set_temperature(
            TempSensType::BoardSensor,
            Temperature::random_full_range().to_celsius().unwrap(),
        )?;
        telemetry.set_temperature(
            TempSensType::ExternalSensor(0),
            Temperature::random_full_range().to_celsius().unwrap(),
        )?;

        // Reset detector after sending telemetry as it is a one-shot event
        self.presence_sensors[0] = false;
        self.presence_sensors[1] = false;

        Ok(telemetry.serialize()?)
    }

    fn command(&mut self, ep: Endpoint, data: &[u8]) -> Result<(), FailCode> {
        if ep != Endpoint::BoardControl {
            return Err(FailCode::ENOTSUP);
        }

        let command = class0::Command::try_from_raw(&data[0..2])?;

        self.lights[0].apply(&command.get_io_xps(IO::Oc1).unwrap());
        self.lights[1].apply(&command.get_io_xps(IO::Oc2).unwrap());
        self.siren.apply(&command.get_io_xps(IO::Relay1).unwrap());

        Ok(())
    }
}

impl NodeApi for OutdoorAlarmController {
    fn app_next_timeout(&self, now: &std::time::Instant) -> Option<Duration> {
        [&self.lights[0], &self.lights[1], &self.siren]
            .iter()
            .ttl(now)
    }

    fn app_process(&mut self, now: &std::time::Instant) -> Option<Endpoint> {
        let mut state_changed = false;
        state_changed |= self.lights[0].pulse_process(now).is_some();
        state_changed |= self.lights[1].pulse_process(now).is_some();
        state_changed |= self.siren.pulse_process(now).is_some();

        if state_changed {
            Some(Endpoint::BoardControl)
        } else {
            None
        }
    }
}
