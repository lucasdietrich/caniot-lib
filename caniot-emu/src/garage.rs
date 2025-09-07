use std::time::{Duration, Instant};

use caniot::{
    class::{
        TempSensType,
        class0::{self, IO},
        llpayload::LLPayload,
    },
    datatypes::{Temperature, Xps},
    device::implementation::{DeviceApi, TelemetryError},
    types::Endpoint,
};
use expirable::Expirable;
use log::debug;

use crate::Node;

#[derive(Default, Debug)]
enum Door {
    #[default]
    Closed,
    Open,
    Opening(Option<Instant>), // Some() is the time the door started opening, None if the door stopped opening
    Closing(Option<Instant>), // Some() is the time the door started closing, None if the door stopped closing
}

impl Door {
    pub const OPENNING_DURATION: Duration = Duration::from_millis(10_000);
    pub const CLOSING_DURATION: Duration = Duration::from_millis(10_000);

    fn pulse_relay(&mut self) {
        debug!("Pulsing relay {:?}", self);

        *self = match self {
            Door::Open => Door::Closing(Some(Instant::now())),
            Door::Closed => Door::Opening(Some(Instant::now())),
            Door::Opening(None) => Door::Closing(Some(Instant::now())),
            Door::Closing(None) => Door::Opening(Some(Instant::now())),
            Door::Opening(Some(_)) => Door::Closing(None),
            Door::Closing(Some(_)) => Door::Opening(None),
        }
    }

    // in milliseconds, 0 if completed
    fn get_time_to_complete_ms(&self, now: &Instant) -> Option<Duration> {
        match self {
            Door::Opening(Some(start)) => {
                let ellapsed = *now - *start;
                if ellapsed >= Self::OPENNING_DURATION {
                    Some(Duration::ZERO)
                } else {
                    Some(Self::OPENNING_DURATION - ellapsed)
                }
            }
            Door::Closing(Some(start)) => {
                let ellapsed = *now - *start;
                if ellapsed >= Self::CLOSING_DURATION {
                    Some(Duration::ZERO)
                } else {
                    Some(Self::CLOSING_DURATION - ellapsed)
                }
            }
            _ => None,
        }
    }

    fn update_state(&mut self, now: &Instant) {
        debug!("Updating state {:?}", self);
        match self {
            Door::Opening(Some(start)) => {
                if *now - *start >= Self::OPENNING_DURATION {
                    debug!("Door opened");
                    *self = Door::Open;
                }
            }
            Door::Closing(Some(start)) => {
                if *now - *start >= Self::CLOSING_DURATION {
                    debug!("Door closed");
                    *self = Door::Closed;
                }
            }
            _ => (),
        }
    }

    fn is_open(&self) -> bool {
        !matches!(self, Door::Closed)
    }
}

impl Expirable<Duration> for Door {
    type Instant = Instant;

    fn ttl(&self, now: &Instant) -> Option<Duration> {
        self.get_time_to_complete_ms(now)
    }
}

pub struct GarageController {
    left_door: Door,  // RL1, IN3
    right_door: Door, // RL2, IN4
    gate_open: bool,  // IN2
}

impl Default for GarageController {
    fn default() -> Self {
        Self {
            left_door: Door::default(),
            right_door: Door::default(),
            gate_open: false,
        }
    }
}

impl Node for GarageController {
    fn next_timeout(&self, now: &Instant) -> Option<Duration> {
        [&self.left_door, &self.right_door].iter().ttl(now)
    }

    fn process(&mut self, now: &Instant) -> Option<Endpoint> {
        self.left_door.update_state(now);
        self.right_door.update_state(now);
        Some(Endpoint::BoardControl)
    }
}

impl DeviceApi for GarageController {
    fn command(&mut self, ep: Endpoint, data: &[u8]) -> Result<(), TelemetryError> {
        if !matches!(ep, Endpoint::BoardControl) {
            return Err(TelemetryError::NotSupported);
        }

        let command = class0::Command::try_from_raw(data).map_err(|_| TelemetryError::Frame)?;

        if command.get_io_xps(IO::Relay1) == Ok(Xps::PulseOn) {
            self.left_door.pulse_relay();
        }

        if command.get_io_xps(IO::Relay2) == Ok(Xps::PulseOn) {
            self.right_door.pulse_relay();
        }

        Ok(())
    }

    fn telemetry(&mut self, ep: Endpoint) -> Result<Vec<u8>, TelemetryError> {
        if !matches!(ep, Endpoint::BoardControl) {
            return Err(TelemetryError::NotSupported);
        }
        let mut telemetry = class0::Telemetry::default();

        telemetry
            .set_io(IO::Input1, true)
            .expect("Failed to set IO");
        telemetry
            .set_io(IO::Input2, self.gate_open)
            .expect("Failed to set IO");
        telemetry
            .set_io(IO::Input3, self.left_door.is_open())
            .expect("Failed to set IO");
        telemetry
            .set_io(IO::Input4, self.right_door.is_open())
            .expect("Failed to set IO");
        telemetry
            .set_temperature(
                TempSensType::BoardSensor,
                Temperature::random_full_range().to_celsius().unwrap(),
            )
            .expect("Failed to set temperature");
        telemetry
            .clear_temperature(TempSensType::ExternalSensor(0))
            .unwrap();
        telemetry
            .clear_temperature(TempSensType::ExternalSensor(1))
            .unwrap();
        telemetry
            .clear_temperature(TempSensType::ExternalSensor(2))
            .unwrap();

        Ok(telemetry.serialize().unwrap())
    }
}
