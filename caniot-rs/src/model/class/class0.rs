use core::ops::{Deref, DerefMut};

use caniot_sys as ll;
use serde::{Deserialize, Serialize};

use crate::{
    class::{
        llpayload::{HasEffect, LLCommand, LLPayload, LLTelemetry}, TelemetryTrait, TempSensType
    }, error::FailCode, Temperature, Xps
};

impl LLPayload for ll::caniot_blc0_telemetry {
    const SER_SIZE: u8 = ll::CANIOT_BLC0_TELEMETRY_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc0_telemetry_defaults;
}

impl LLTelemetry for ll::caniot_blc0_telemetry {
    type IOType = ll::caniot_blc0_io_t::Type;

    const GET_TEMP_FN: unsafe extern "C" fn(
        t: *const Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
        temperature: *mut u16,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_get_temperature;

    const SET_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
        temperature: u16,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_set_temperature;

    const CLEAR_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_clear_temperature;

    const GET_IO_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        state: *mut bool,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_get_io;

    const SET_IO_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        state: bool,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_telemetry_set_io;
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Telemetry(ll::caniot_blc0_telemetry);

impl Deref for Telemetry {
    type Target = ll::caniot_blc0_telemetry;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for Telemetry {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
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

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct TelemetryData {
    pub oc1: bool,
    pub oc2: bool,
    pub rl1: bool,
    pub rl2: bool,
    pub in1: bool,
    pub in2: bool,
    pub in3: bool,
    pub in4: bool,
    pub poc1: bool,
    pub poc2: bool,
    pub prl1: bool,
    pub prl2: bool,

    pub temp_in: Temperature,
    pub temp_out: [Temperature; 3],
}

impl Telemetry {
    pub fn to_telemetry_data(&self) -> Result<TelemetryData, FailCode> {
        Ok(TelemetryData {
            oc1: self.get_io(IO::Oc1)?,
            oc2: self.get_io(IO::Oc2)?,
            rl1: self.get_io(IO::Relay1)?,
            rl2: self.get_io(IO::Relay2)?,
            in1: self.get_io(IO::Input1)?,
            in2: self.get_io(IO::Input2)?,
            in3: self.get_io(IO::Input3)?,
            in4: self.get_io(IO::Input4)?,
            poc1: self.get_io(IO::Oc1PulseActive)?,
            poc2: self.get_io(IO::Oc2PulseActive)?,
            prl1: self.get_io(IO::Relay1PulseActive)?,
            prl2: self.get_io(IO::Relay2PulseActive)?,
            temp_in: self.get_temperature(TempSensType::BoardSensor)?,
            temp_out: [
                self.get_temperature(TempSensType::ExternalSensor(0))?,
                self.get_temperature(TempSensType::ExternalSensor(1))?,
                self.get_temperature(TempSensType::ExternalSensor(2))?,
            ],
        })
    }

    pub fn from_telemetry_data(data: TelemetryData) -> Result<Self, FailCode> {
        let mut t = Telemetry::default();
        t.set_io(IO::Oc1, data.oc1)?;
        t.set_io(IO::Oc2, data.oc2)?;
        t.set_io(IO::Relay1, data.rl1)?;
        t.set_io(IO::Relay2, data.rl2)?;
        t.set_io(IO::Input1, data.in1)?;
        t.set_io(IO::Input2, data.in2)?;
        t.set_io(IO::Input3, data.in3)?;
        t.set_io(IO::Input4, data.in4)?;
        t.set_io(IO::Oc1PulseActive, data.poc1)?;
        t.set_io(IO::Oc2PulseActive, data.poc2)?;
        t.set_io(IO::Relay1PulseActive, data.prl1)?;
        t.set_io(IO::Relay2PulseActive, data.prl2)?;
        t.set_temperature(TempSensType::BoardSensor, data.temp_in)?;
        t.set_temperature(TempSensType::ExternalSensor(0), data.temp_out[0])?;
        t.set_temperature(TempSensType::ExternalSensor(1), data.temp_out[1])?;
        t.set_temperature(TempSensType::ExternalSensor(2), data.temp_out[2])?;
        Ok(t)
    }
}

impl TelemetryTrait for TelemetryData {
    fn get_temperature(&self, sensor: TempSensType) -> Option<Temperature> {
        match sensor {
            TempSensType::BoardSensor => Some(self.temp_in),
            TempSensType::ExternalSensor(n) if n < 3 => Some(self.temp_out[n as usize]),
            _ => None,
        }
    }
}

impl LLPayload for ll::caniot_blc0_command {
    const SER_SIZE: u8 = ll::CANIOT_BLC0_COMMAND_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc0_command_defaults;
}

impl HasEffect for ll::caniot_blc0_command {}

impl LLCommand for ll::caniot_blc0_command {
    type IOType = ll::caniot_blc0_io_t::Type;

    const GET_IO_XPS_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        xps: *mut ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_get_xps;

    const SET_IO_XPS_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        xps: ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc0_command_set_xps;
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

impl DerefMut for Command {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl Default for Command {
    fn default() -> Self {
        Command(ll::caniot_blc0_command::default())
    }
}

impl Command {
    pub fn try_from_raw(data: impl AsRef<[u8]>) -> Result<Self, FailCode> {
        let command = ll::caniot_blc0_command::deserialize(data)?;
        Ok(Command(command))
    }
}

pub struct CommandData {
    pub oc1: Xps,
    pub oc2: Xps,
    pub rl1: Xps,
    pub rl2: Xps,
}

impl Command {
    pub fn to_command_data(&self) -> Result<CommandData, FailCode> {
        Ok(CommandData {
            oc1: self.get_io_xps(IO::Oc1)?,
            oc2: self.get_io_xps(IO::Oc2)?,
            rl1: self.get_io_xps(IO::Relay1)?,
            rl2: self.get_io_xps(IO::Relay2)?,
        })
    }

    pub fn from_command_data(data: CommandData) -> Result<Self, FailCode> {
        let mut cmd = Command::default();
        cmd.set_io_xps(IO::Oc1, data.oc1)?;
        cmd.set_io_xps(IO::Oc2, data.oc2)?;
        cmd.set_io_xps(IO::Relay1, data.rl1)?;
        cmd.set_io_xps(IO::Relay2, data.rl2)?;
        Ok(cmd)
    }
}
