use core::ops::{Deref, DerefMut};

use caniot_sys as ll;
use serde::{Deserialize, Serialize};

use crate::{
    class::{
        llpayload::{HasEffect, LLCommand, LLPayload, LLTelemetry}, TelemetryTrait, TempSensType
    }, error::FailCode, Temperature, Xps
};

impl LLPayload for ll::caniot_blc1_telemetry {
    const SER_SIZE: u8 = ll::CANIOT_BLC1_TELEMETRY_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc1_telemetry_defaults;
}

impl LLTelemetry for ll::caniot_blc1_telemetry {
    type IOType = ll::caniot_blc1_io_t::Type;

    const GET_TEMP_FN: unsafe extern "C" fn(
        t: *const Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
        temperature: *mut u16,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_get_temperature;

    const SET_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
        temperature: u16,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_set_temperature;

    const CLEAR_TEMP_FN: unsafe extern "C" fn(
        t: *mut Self,
        sensor: caniot_sys::caniot_temp_sens_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_clear_temperature;

    const GET_IO_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        state: *mut bool,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_get_io;

    const SET_IO_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        state: bool,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_telemetry_set_io;
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Telemetry(ll::caniot_blc1_telemetry);

impl Deref for Telemetry {
    type Target = ll::caniot_blc1_telemetry;

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
        Telemetry(ll::caniot_blc1_telemetry::default())
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum IO {
    Pc0,
    Pc1,
    Pc2,
    Pc3,
    Pd4,
    Pd5,
    Pd6,
    Pd7,
    Eio0,
    Eio1,
    Eio2,
    Eio3,
    Eio4,
    Eio5,
    Eio6,
    Eio7,
    Pb0,
    Pe0,
    Pe1,
}

impl TryFrom<ll::caniot_blc1_io_t::Type> for IO {
    type Error = FailCode;

    fn try_from(value: ll::caniot_blc1_io_t::Type) -> Result<Self, Self::Error> {
        match value {
            ll::caniot_blc1_io_t::CANIOT_BLC1_PC0 => Ok(IO::Pc0),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PC1 => Ok(IO::Pc1),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PC2 => Ok(IO::Pc2),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PC3 => Ok(IO::Pc3),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PD4 => Ok(IO::Pd4),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PD5 => Ok(IO::Pd5),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PD6 => Ok(IO::Pd6),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PD7 => Ok(IO::Pd7),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO0 => Ok(IO::Eio0),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO1 => Ok(IO::Eio1),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO2 => Ok(IO::Eio2),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO3 => Ok(IO::Eio3),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO4 => Ok(IO::Eio4),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO5 => Ok(IO::Eio5),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO6 => Ok(IO::Eio6),
            ll::caniot_blc1_io_t::CANIOT_BLC1_EIO7 => Ok(IO::Eio7),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PB0 => Ok(IO::Pb0),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PE0 => Ok(IO::Pe0),
            ll::caniot_blc1_io_t::CANIOT_BLC1_PE1 => Ok(IO::Pe1),
            _ => Err(FailCode::EINVAL),
        }
    }
}

impl TryFrom<IO> for ll::caniot_blc1_io_t::Type {
    type Error = FailCode;

    fn try_from(value: IO) -> Result<Self, Self::Error> {
        match value {
            IO::Pc0 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PC0),
            IO::Pc1 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PC1),
            IO::Pc2 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PC2),
            IO::Pc3 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PC3),
            IO::Pd4 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PD4),
            IO::Pd5 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PD5),
            IO::Pd6 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PD6),
            IO::Pd7 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PD7),
            IO::Eio0 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO0),
            IO::Eio1 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO1),
            IO::Eio2 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO2),
            IO::Eio3 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO3),
            IO::Eio4 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO4),
            IO::Eio5 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO5),
            IO::Eio6 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO6),
            IO::Eio7 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_EIO7),
            IO::Pb0 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PB0),
            IO::Pe0 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PE0),
            IO::Pe1 => Ok(ll::caniot_blc1_io_t::CANIOT_BLC1_PE1),
        }
    }
}

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct TelemetryData {
    pub ios: [bool; 19],

    pub temp_in: Temperature,
    pub temp_out: [Temperature; 3],
}

impl Telemetry {
    pub fn to_telemetry_data(&self) -> Result<TelemetryData, FailCode> {
        Ok(TelemetryData {
            ios: [
                self.get_io(IO::Pc0)?,
                self.get_io(IO::Pc1)?,
                self.get_io(IO::Pc2)?,
                self.get_io(IO::Pc3)?,
                self.get_io(IO::Pd4)?,
                self.get_io(IO::Pd5)?,
                self.get_io(IO::Pd6)?,
                self.get_io(IO::Pd7)?,
                self.get_io(IO::Eio0)?,
                self.get_io(IO::Eio1)?,
                self.get_io(IO::Eio2)?,
                self.get_io(IO::Eio3)?,
                self.get_io(IO::Eio4)?,
                self.get_io(IO::Eio5)?,
                self.get_io(IO::Eio6)?,
                self.get_io(IO::Eio7)?,
                self.get_io(IO::Pb0)?,
                self.get_io(IO::Pe0)?,
                self.get_io(IO::Pe1)?,
            ],

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
        t.set_io(IO::Pc0, data.ios[0])?;
        t.set_io(IO::Pc1, data.ios[1])?;
        t.set_io(IO::Pc2, data.ios[2])?;
        t.set_io(IO::Pc3, data.ios[3])?;
        t.set_io(IO::Pd4, data.ios[4])?;
        t.set_io(IO::Pd5, data.ios[5])?;
        t.set_io(IO::Pd6, data.ios[6])?;
        t.set_io(IO::Pd7, data.ios[7])?;
        t.set_io(IO::Eio0, data.ios[8])?;
        t.set_io(IO::Eio1, data.ios[9])?;
        t.set_io(IO::Eio2, data.ios[10])?;
        t.set_io(IO::Eio3, data.ios[11])?;
        t.set_io(IO::Eio4, data.ios[12])?;
        t.set_io(IO::Eio5, data.ios[13])?;
        t.set_io(IO::Eio6, data.ios[14])?;
        t.set_io(IO::Eio7, data.ios[15])?;
        t.set_io(IO::Pb0, data.ios[16])?;
        t.set_io(IO::Pe0, data.ios[17])?;
        t.set_io(IO::Pe1, data.ios[18])?;

        t.set_temperature(TempSensType::BoardSensor, data.temp_in)?;
        t.set_temperature(TempSensType::ExternalSensor(0), data.temp_out[0])?;
        t.set_temperature(TempSensType::ExternalSensor(1), data.temp_out[1])?;
        t.set_temperature(TempSensType::ExternalSensor(2), data.temp_out[2])?;

        Ok(t)
    }
}

impl LLPayload for ll::caniot_blc1_command {
    const SER_SIZE: u8 = ll::CANIOT_BLC1_COMMAND_BUF_LEN as u8;

    const SER_FN: unsafe extern "C" fn(
        t: *const Self,
        buf: *mut u8,
        len: *mut u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_ser;

    const DESER_FN: unsafe extern "C" fn(
        t: *mut Self,
        buf: *const u8,
        len: u8,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_get;

    const DEFAULT_FN: unsafe extern "C" fn(t: *mut Self) -> ::core::ffi::c_int =
        ll::caniot_blc1_command_defaults;
}

impl HasEffect for ll::caniot_blc1_command {}

impl LLCommand for ll::caniot_blc1_command {
    type IOType = ll::caniot_blc1_io_t::Type;

    const GET_IO_XPS_FN: unsafe extern "C" fn(
        t: *const Self,
        io: Self::IOType,
        xps: *mut ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_get_xps;

    const SET_IO_XPS_FN: unsafe extern "C" fn(
        t: *mut Self,
        io: Self::IOType,
        xps: ll::caniot_complex_digital_cmd_t::Type,
    ) -> ::core::ffi::c_int = ll::caniot_blc1_command_set_xps;
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Command(ll::caniot_blc1_command);

impl Deref for Command {
    type Target = ll::caniot_blc1_command;

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
        Command(ll::caniot_blc1_command::default())
    }
}

impl Command {
    pub fn try_from_raw(data: impl AsRef<[u8]>) -> Result<Self, FailCode> {
        let command = ll::caniot_blc1_command::deserialize(data)?;
        Ok(Command(command))
    }
}

pub struct CommandData {
    pub ios: [Xps; 19],
}

impl Command {
    pub fn to_command_data(&self) -> Result<CommandData, FailCode> {
        Ok(CommandData {
            ios: [
                self.get_io_xps(IO::Pc0)?,
                self.get_io_xps(IO::Pc1)?,
                self.get_io_xps(IO::Pc2)?,
                self.get_io_xps(IO::Pc3)?,
                self.get_io_xps(IO::Pd4)?,
                self.get_io_xps(IO::Pd5)?,
                self.get_io_xps(IO::Pd6)?,
                self.get_io_xps(IO::Pd7)?,
                self.get_io_xps(IO::Eio0)?,
                self.get_io_xps(IO::Eio1)?,
                self.get_io_xps(IO::Eio2)?,
                self.get_io_xps(IO::Eio3)?,
                self.get_io_xps(IO::Eio4)?,
                self.get_io_xps(IO::Eio5)?,
                self.get_io_xps(IO::Eio6)?,
                self.get_io_xps(IO::Eio7)?,
                self.get_io_xps(IO::Pb0)?,
                self.get_io_xps(IO::Pe0)?,
                self.get_io_xps(IO::Pe1)?,
            ],
        })
    }

    pub fn from_command_data(data: CommandData) -> Result<Self, FailCode> {
        let mut c = Command::default();
        c.set_io_xps(IO::Pc0, data.ios[0])?;
        c.set_io_xps(IO::Pc1, data.ios[1])?;
        c.set_io_xps(IO::Pc2, data.ios[2])?;
        c.set_io_xps(IO::Pc3, data.ios[3])?;
        c.set_io_xps(IO::Pd4, data.ios[4])?;
        c.set_io_xps(IO::Pd5, data.ios[5])?;
        c.set_io_xps(IO::Pd6, data.ios[6])?;
        c.set_io_xps(IO::Pd7, data.ios[7])?;
        c.set_io_xps(IO::Eio0, data.ios[8])?;
        c.set_io_xps(IO::Eio1, data.ios[9])?;
        c.set_io_xps(IO::Eio2, data.ios[10])?;
        c.set_io_xps(IO::Eio3, data.ios[11])?;
        c.set_io_xps(IO::Eio4, data.ios[12])?;
        c.set_io_xps(IO::Eio5, data.ios[13])?;
        c.set_io_xps(IO::Eio6, data.ios[14])?;
        c.set_io_xps(IO::Eio7, data.ios[15])?;
        c.set_io_xps(IO::Pb0, data.ios[16])?;
        c.set_io_xps(IO::Pe0, data.ios[17])?;
        c.set_io_xps(IO::Pe1, data.ios[18])?;
        Ok(c)
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