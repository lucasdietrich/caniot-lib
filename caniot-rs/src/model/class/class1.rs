use core::ops::Deref;

use caniot_sys as ll;

use crate::{
    class::{TempSensType, llpayload::LLPayload},
    datatypes::{Temperature, Xps},
    error::FailCode,
};

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct Telemetry(ll::caniot_blc1_telemetry);

impl Deref for Telemetry {
    type Target = ll::caniot_blc1_telemetry;

    fn deref(&self) -> &Self::Target {
        &self.0
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

impl Telemetry {
    pub fn set_temperature(&mut self, sensor: TempSensType, celsius: f32) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let temperature = Temperature::from_celsius(celsius);

        let ret = unsafe {
            ll::caniot_blc1_telemetry_set_temperature(&mut self.0, sensor, temperature.to_raw_u10())
        };
        FailCode::to_errno(ret)
    }

    pub fn clear_temperature(&mut self, sensor: TempSensType) -> Result<(), FailCode> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor)?;
        let ret = unsafe { ll::caniot_blc1_telemetry_clear_temperature(&mut self.0, sensor) };
        FailCode::to_errno(ret)
    }

    pub fn get_temperature(&self, sensor: TempSensType) -> Option<f32> {
        let sensor = ll::caniot_temp_sens_t::Type::try_from(sensor).ok()?;
        let mut temperature: u16 = 0;
        let ret =
            unsafe { ll::caniot_blc1_telemetry_get_temperature(&self.0, sensor, &mut temperature) };
        FailCode::to_errno(ret).ok()?;
        Temperature::from_raw_u10(temperature).to_celsius()
    }

    pub fn set_io(&mut self, io: IO, state: bool) -> Result<(), FailCode> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io)?;
        let ret = unsafe { ll::caniot_blc1_telemetry_set_io(&mut self.0, io, state) };
        FailCode::to_errno(ret)
    }

    pub fn get_io(&self, io: IO) -> Option<bool> {
        let io = ll::caniot_blc0_io_t::Type::try_from(io).ok()?;
        let mut state = false;
        let ret = unsafe { ll::caniot_blc1_telemetry_get_io(&self.0, io, &mut state) };
        FailCode::to_result(ret).ok()?;
        Some(state)
    }
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

impl Default for Command {
    fn default() -> Self {
        Command(ll::caniot_blc1_command::default())
    }
}

impl Command {
    pub fn try_from_raw(data: impl AsRef<[u8]>) -> Result<Self, FailCode> {
        let command = ll::caniot_blc1_command::deserialize(data.as_ref())?;
        Ok(Command(command))
    }

    pub fn set_io_xps(&mut self, io: IO, xps: Xps) -> Result<(), FailCode> {
        let io = ll::caniot_blc1_io_t::Type::try_from(io)?;
        let xps = xps.into();
        let ret = unsafe { ll::caniot_blc1_command_set_xps(&mut self.0, io, xps) };
        FailCode::to_errno(ret)
    }

    pub fn get_io_xps(&self, io: IO) -> Result<Xps, FailCode> {
        let io = ll::caniot_blc1_io_t::Type::try_from(io)?;
        let mut xps = ll::caniot_complex_digital_cmd_t::CANIOT_XPS_NONE;
        let ret = unsafe { ll::caniot_blc1_command_get_xps(&self.0, io, &mut xps) };
        FailCode::to_errno(ret)?;
        Ok(Xps::from(xps))
    }
}
