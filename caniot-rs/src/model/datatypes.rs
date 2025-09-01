use core::{
    cmp::{max, min},
    fmt::{Debug, Display},
};

use num_derive::{FromPrimitive, ToPrimitive};
use num_traits::FromPrimitive;

#[derive(Clone, Copy, PartialEq, Default)]
pub struct Temperature(Option<i16>);

impl Temperature {
    pub const INVALID: Temperature = Temperature(None);
    const VALUE_U10_MASK: u16 = 0x3FF;
    const VALUE_U10_INVALID_MARKER1: u16 = Self::VALUE_U10_MASK;
    const VALUE_U10_INVALID_MARKER2: u16 = 0;
    const VALUE_U10_MAX_VALID: u16 = 1000;
    const VALUE_U16_MASK: u16 = 0xFFFF;
    const VALUE_U16_INVALID_MARKER1: u16 = Self::VALUE_U16_MASK;
    const VALUE_U16_INVALID_MARKER2: u16 = 0;

    const VALUE_F_MIN: f32 = -28.0;
    const VALUE_F_MAX: f32 = 72.0;
    const VALUE_I16_MIN: i16 = -2800;
    const VALUE_I16_MAX: i16 = 7200;

    pub fn new(val: i16) -> Self {
        if val < Self::VALUE_I16_MIN || val > Self::VALUE_I16_MAX {
            Self::INVALID
        } else {
            Temperature(Some(val))
        }
    }

    pub fn from_raw_u16(raw: u16) -> Self {
        if raw == Self::VALUE_U16_INVALID_MARKER1 || raw == Self::VALUE_U16_INVALID_MARKER2 {
            Self::INVALID
        } else {
            Temperature(Some(raw as i16))
        }
    }

    pub fn from_raw_u10(raw: u16) -> Self {
        if raw == Self::VALUE_U10_INVALID_MARKER1
            || raw == Self::VALUE_U10_INVALID_MARKER2
            || raw > Self::VALUE_U10_MAX_VALID
        {
            Self::INVALID
        } else {
            Temperature(Some((raw * 10) as i16 - 2800))
        }
    }

    pub fn to_raw_u16(&self) -> u16 {
        match self.0 {
            Some(val) => val as u16,
            None => Self::VALUE_U16_MASK,
        }
    }

    pub fn to_raw_u16_bytes(&self) -> [u8; 2] {
        self.to_raw_u16().to_le_bytes()
    }

    pub fn to_raw_u10(&self) -> u16 {
        match self.0 {
            Some(val) => {
                let val = val / 10;
                let val = max(min(val, 720), -280) as i16;
                (val + 280) as u16
            }
            None => Self::VALUE_U10_INVALID_MARKER1,
        }
    }

    pub fn to_raw_u10_bytes(&self) -> [u8; 2] {
        self.to_raw_u10().to_le_bytes()
    }

    pub fn to_celsius(&self) -> Option<f32> {
        match self.0 {
            Some(val) => Some(val as f32 / 100.0),
            None => None,
        }
    }

    pub fn from_celsius(val: f32) -> Self {
        if val < Self::VALUE_F_MIN || val > Self::VALUE_F_MAX {
            Self::INVALID
        } else {
            Temperature(Some((val * 100.0) as i16))
        }
    }

    #[cfg(feature = "std")]
    pub fn random_full_range() -> Self {
        let rand = rand::random::<u16>() % Self::VALUE_U10_MAX_VALID;
        Temperature::from_raw_u10(rand)
    }

    #[cfg(feature = "std")]
    pub fn random_range(min: f32, max: f32) -> Self {
        let rand = rand::random::<u16>() % Self::VALUE_U10_MAX_VALID;
        let val = min + (max - min) * (rand as f32 / Self::VALUE_U10_MAX_VALID as f32);
        Temperature::from_celsius(val)
    }

    pub fn invalid() -> Self {
        Self::INVALID
    }
}

impl Display for Temperature {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self.to_celsius() {
            Some(val) => write!(f, "{} °C", val),
            None => write!(f, "INVALID °C"),
        }
    }
}

impl Debug for Temperature {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self.to_celsius() {
            Some(val) => write!(f, "{} °C (raw {})", val, self.0.unwrap()),
            None => write!(f, "INVALID °C"),
        }
    }
}

impl TryFrom<Temperature> for f32 {
    type Error = ();

    fn try_from(value: Temperature) -> Result<Self, Self::Error> {
        match value.to_celsius() {
            Some(val) => Ok(val),
            None => Err(()),
        }
    }
}

#[derive(Debug, Default, Clone, Copy, PartialEq, FromPrimitive)]
pub enum Xps {
    #[default]
    None = 0,
    SetOn = 1,
    SetOff = 2,
    Toggle = 3,
    Reset = 4,
    PulseOn = 5,
    PulseOff = 6,
    PulseCancel = 7,
}

impl Xps {
    pub fn set_at(&self, data: &mut [u8], position: usize) {
        let len = data.len();
        let lsb_index = position * 3;
        let lsb_offset = lsb_index & 0x7;
        let lsb_available_size = 8 - lsb_offset;
        let byte_n = lsb_index >> 3;
        let xps = *self as u8;
        data[byte_n] |= (xps << lsb_offset) as u8;

        if lsb_available_size < 3 && (byte_n + 1) < len {
            data[byte_n + 1] |= xps >> lsb_available_size;
        }
    }

    pub fn get_at(data: &[u8], position: usize) -> Result<Self, &'static str> {
        let len = data.len();
        let lsb_index = position * 3;
        let lsb_offset = lsb_index & 0x7;
        let lsb_available_size = 8 - lsb_offset;
        let byte_n = lsb_index >> 3;
        let mut xps = ((data[byte_n] >> lsb_offset) & 0x7) as u8;

        if lsb_available_size < 3 && (byte_n + 1) < len {
            let msb_remaining_size = 3 - lsb_available_size;
            let mask = 0xFF >> (8 - msb_remaining_size);
            xps |= (data[byte_n + 1] & mask) << lsb_available_size;
        }

        match Xps::from_u8(xps) {
            Some(xps) => Ok(xps),
            None => Err("PayloadDecodeError"),
        }
    }
}

#[derive(Debug, Default, Clone, Copy, PartialEq, FromPrimitive)]
pub enum TS {
    #[default]
    None = 0,
    Set = 1,
    Reset = 2,
    Toggle = 3,
}

#[derive(Debug, Default, Clone, Copy, PartialEq, FromPrimitive)]
pub enum TSP {
    #[default]
    None = 0,
    Set = 1,
    Reset = 2,
    Pulse = 3,
}

#[derive(Debug, Default, Clone, Copy, PartialEq, FromPrimitive, ToPrimitive)]
pub enum HeatingMode {
    #[default]
    None = 0,
    Comfort = 1,
    ComfortMin1 = 2,
    ComfortMin2 = 3,
    EnergySaving = 4,
    FrostProtection = 5,
    Stop = 6,
    // unused
}

impl HeatingMode {
    pub fn heater_on(&self, include_low_power_modes: bool) -> bool {
        match self {
            HeatingMode::None | HeatingMode::Stop => false,
            HeatingMode::Comfort | HeatingMode::ComfortMin1 | HeatingMode::ComfortMin2 => true,
            HeatingMode::EnergySaving | HeatingMode::FrostProtection => include_low_power_modes,
        }
    }
}
