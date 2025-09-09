use core::ops::Deref;

use caniot_sys as ll;

use crate::{
    class::{TempSensType, llpayload::LLPayload},
    datatypes::{Temperature, Xps},
    error::FailCode,
};

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Telemetry(ll::caniot_blc0_telemetry);

impl Deref for Telemetry {
    type Target = ll::caniot_blc0_telemetry;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl Default for Telemetry {
    fn default() -> Self {
        Telemetry(ll::caniot_blc0_telemetry::default())
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum IO {
    Oc1,
    Oc2,
    Relay1,
    Relay2,
    Input1,
    Input2,
    Input3,
    Input4,
    Oc1PulseActive,
    Oc2PulseActive,
    Relay1PulseActive,
    Relay2PulseActive,
}

impl TryFrom<ll::caniot_blc0_io_t::Type> for IO {
    type Error = FailCode;

    fn try_from(value: ll::caniot_blc0_io_t::Type) -> Result<Self, Self::Error> {
        match value {
            ll::caniot_blc0_io_t::CANIOT_BLC0_OC1 => Ok(IO::Oc1),
            ll::caniot_blc0_io_t::CANIOT_BLC0_OC2 => Ok(IO::Oc2),
            ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY1 => Ok(IO::Relay1),
            ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY2 => Ok(IO::Relay2),
            ll::caniot_blc0_io_t::CANIOT_BLC0_IN1 => Ok(IO::Input1),
            ll::caniot_blc0_io_t::CANIOT_BLC0_IN2 => Ok(IO::Input2),
            ll::caniot_blc0_io_t::CANIOT_BLC0_IN3 => Ok(IO::Input3),
            ll::caniot_blc0_io_t::CANIOT_BLC0_IN4 => Ok(IO::Input4),
            ll::caniot_blc0_io_t::CANIOT_BLC0_OC1_PULSE_ACTIVE => Ok(IO::Oc1PulseActive),
            ll::caniot_blc0_io_t::CANIOT_BLC0_OC2_PULSE_ACTIVE => Ok(IO::Oc2PulseActive),
            ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY1_PULSE_ACTIVE => Ok(IO::Relay1PulseActive),
            ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY2_PULSE_ACTIVE => Ok(IO::Relay2PulseActive),
            _ => Err(FailCode::EINVAL),
        }
    }
}

impl TryFrom<IO> for ll::caniot_blc0_io_t::Type {
    type Error = FailCode;

    fn try_from(value: IO) -> Result<Self, Self::Error> {
        match value {
            IO::Oc1 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_OC1),
            IO::Oc2 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_OC2),
            IO::Relay1 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY1),
            IO::Relay2 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY2),
            IO::Input1 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_IN1),
            IO::Input2 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_IN2),
            IO::Input3 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_IN3),
            IO::Input4 => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_IN4),
            IO::Oc1PulseActive => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_OC1_PULSE_ACTIVE),
            IO::Oc2PulseActive => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_OC2_PULSE_ACTIVE),
            IO::Relay1PulseActive => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY1_PULSE_ACTIVE),
            IO::Relay2PulseActive => Ok(ll::caniot_blc0_io_t::CANIOT_BLC0_RELAY2_PULSE_ACTIVE),
        }
    }
}

impl Telemetry {
    pub fn set_temperature(&mut self, sensor: TempSensType, celsius: f32) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let temperature = Temperature::from_celsius(celsius);

        let ret = unsafe {
            ll::caniot_blc0_telemetry_set_temperature(&mut self.0, sensor, temperature.to_raw_u10())
        };
        FailCode::to_errno(ret)
    }

    pub fn clear_temperature(&mut self, sensor: TempSensType) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let ret = unsafe { ll::caniot_blc0_telemetry_clear_temperature(&mut self.0, sensor) };
        FailCode::to_errno(ret)
    }

    pub fn get_temperature(&self, sensor: TempSensType) -> Option<f32> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor).ok()?;
        let mut temperature: u16 = 0;
        let ret =
            unsafe { ll::caniot_blc0_telemetry_get_temperature(&self.0, sensor, &mut temperature) };
        FailCode::to_errno(ret).ok()?;
        Temperature::from_raw_u10(temperature).to_celsius()
    }

    pub fn set_io(&mut self, io: IO, state: bool) -> Result<(), FailCode> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io)?;
        let ret = unsafe { ll::caniot_blc0_telemetry_set_io(&mut self.0, io, state) };
        FailCode::to_errno(ret)
    }

    pub fn get_io(&self, io: IO) -> Option<bool> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io).ok()?;
        let mut state = false;
        let ret = unsafe { ll::caniot_blc0_telemetry_get_io(&self.0, io, &mut state) };
        FailCode::to_result(ret).ok()?;
        Some(state)
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Command(ll::caniot_blc0_command);

impl Deref for Command {
    type Target = ll::caniot_blc0_command;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl Default for Command {
    fn default() -> Self {
        Command(ll::caniot_blc0_command::default())
    }
}

impl Command {
    pub fn try_from_raw(data: impl AsRef<[u8]>) -> Result<Self, FailCode> {
        let command = ll::caniot_blc0_command::deserialize(data.as_ref())?;
        Ok(Command(command))
    }

    pub fn set_io_xps(&mut self, io: IO, xps: Xps) -> Result<(), FailCode> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io)?;
        let xps = xps.into();
        let ret = unsafe { ll::caniot_blc0_command_set_xps(&mut self.0, io, xps) };
        FailCode::to_errno(ret)
    }

    pub fn get_io_xps(&self, io: IO) -> Result<Xps, FailCode> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io)?;
        let mut xps = ll::caniot_complex_digital_cmd_t::CANIOT_XPS_NONE;
        let ret = unsafe { ll::caniot_blc0_command_get_xps(&self.0, io, &mut xps) };
        FailCode::to_errno(ret)?;
        Ok(Xps::from(xps))
    }
}
