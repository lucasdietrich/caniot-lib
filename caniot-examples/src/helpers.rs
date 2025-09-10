use std::time::{Duration, Instant};

use caniot::datatypes::Xps;
use expirable::Expirable;

#[derive(Debug)]
pub struct EmuXps {
    pin_state: bool,
    pin_default: bool,
    pulse_time: Option<Instant>,
    pulse_duration: Option<Duration>,
}

impl Default for EmuXps {
    fn default() -> Self {
        EmuXps::new(false, false, None)
    }
}

impl EmuXps {
    pub fn new(initial_state: bool, pin_default: bool, pulse_duration: Option<Duration>) -> Self {
        Self {
            pin_state: initial_state,
            pin_default,
            pulse_time: None,
            pulse_duration,
        }
    }

    pub fn supports_pulse(&self) -> bool {
        self.pulse_duration.is_some()
    }

    pub fn pulse_pending(&self) -> bool {
        self.pulse_time.is_some()
    }

    pub fn pulse_expired(&self, now: &Instant) -> bool {
        if let Some(pulse_time) = self.pulse_time {
            *now - pulse_time >= self.pulse_duration.unwrap()
        } else {
            false
        }
    }

    pub fn time_to_pulse_expire(&self, now: &Instant) -> Option<Duration> {
        if let Some(pulse_time) = self.pulse_time {
            let remaining = self.pulse_duration.unwrap().checked_sub(*now - pulse_time);
            if let Some(remaining) = remaining {
                Some(remaining)
            } else {
                Some(Duration::from_secs(0))
            }
        } else {
            None
        }
    }

    pub fn pulse_process(&mut self, now: &Instant) -> Option<bool> {
        if self.pulse_expired(now) {
            self.pulse_time = None;
            self.pin_state = self.pin_default;
            Some(self.pin_state)
        } else {
            None
        }
    }

    pub fn get_state(&self) -> bool {
        self.pin_state
    }

    pub fn pulse(&mut self, pulse_state: bool) {
        if self.supports_pulse() {
            self.pin_state = pulse_state;
            self.pulse_time = Some(Instant::now());
        }
    }

    pub fn apply(&mut self, action: Xps) {
        match action {
            Xps::None => {}
            Xps::SetOn => {
                self.pin_state = true;
            }
            Xps::SetOff => {
                self.pin_state = false;
            }
            Xps::Toggle => {
                self.pin_state = !self.pin_state;
            }
            Xps::Reset => {
                self.pin_state = self.pin_default;
                self.pulse_time = None;
            }
            Xps::PulseOn => {
                self.pulse(true);
            }
            Xps::PulseOff => {
                self.pulse(false);
            }
            Xps::PulseCancel => {
                self.pulse_time = None;
            }
        }
    }
}

impl Expirable<Duration> for EmuXps {
    type Instant = Instant;

    fn ttl(&self, now: &Instant) -> Option<Duration> {
        self.time_to_pulse_expire(now)
    }
}
