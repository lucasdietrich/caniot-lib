use num_derive::FromPrimitive;

use super::ProtocolError;

#[repr(u16)]
#[derive(Debug, Clone, Copy, PartialEq, FromPrimitive)]
pub enum Attribute {
    NodeId = 0x0000,
    Version = 0x0010,
    Name = 0x0020,
    MagicNumber = 0x0030,
    BuildDate = 0x0040,
    BuildCommit = 0x0050,
    Features = 0x0060,
    SystemUptimeSynced = 0x1000,
    SystemTime = 0x1010,
    SystemUptime = 0x1020,
    SystemStartTime = 0x1030,
    SystemLastTelemetry = 0x1040,
    SystemLastTelemetryMsMod = 0x10B0,
    SystemReceivedTotal = 0x1050,
    SystemReceivedReadAttr = 0x1060,
    SystemReceivedWriteAttr = 0x1070,
    SystemReceivedCommand = 0x1080,
    SystemReceivedReqTelemetry = 0x1090,
    SystemReceivedIgnored = 0x10A0,
    SystemSentTotal = 0x10C0,
    SystemSentTelemetry = 0x10D0,
    SystemUnused4 = 0x10E0,
    SystemLastCommandError = 0x10F0,
    SystemLastTelemetryError = 0x1100,
    SystemUnused5 = 0x1110,
    SystemBattery = 0x1120,
    ConfigTelemetryPeriod = 0x2000,
    ConfigTelemetryDelay = 0x2010,
    ConfigTelemetryDelayMin = 0x2020,
    ConfigTelemetryDelayMax = 0x2030,
    ConfigFlags = 0x2040,
    ConfigTimezone = 0x2050,
    ConfigLocation = 0x2060,
    ConfigCls0GpioPulseDurationOc1 = 0x2070,
    ConfigCls0GpioPulseDurationOc2 = 0x2080,
    ConfigCls0GpioPulseDurationRl1 = 0x2090,
    ConfigCls0GpioPulseDurationRl2 = 0x20A0,
    ConfigCls0GpioOutputsDefault = 0x20B0,
    ConfigCls0GpioMaskTelemetryOnChange = 0x20C0,
    ConfigCls1GpioPulseDurationPc0 = 0x20D0,
    ConfigCls1GpioPulseDurationPc1 = 0x20E0,
    ConfigCls1GpioPulseDurationPc2 = 0x20F0,
    ConfigCls1GpioPulseDurationPc3 = 0x2100,
    ConfigCls1GpioPulseDurationPd0 = 0x2110,
    ConfigCls1GpioPulseDurationPd1 = 0x2120,
    ConfigCls1GpioPulseDurationPd2 = 0x2130,
    ConfigCls1GpioPulseDurationPd3 = 0x2140,
    ConfigCls1GpioPulseDurationPei0 = 0x2150,
    ConfigCls1GpioPulseDurationPei1 = 0x2160,
    ConfigCls1GpioPulseDurationPei2 = 0x2170,
    ConfigCls1GpioPulseDurationPei3 = 0x2180,
    ConfigCls1GpioPulseDurationPei4 = 0x2190,
    ConfigCls1GpioPulseDurationPei5 = 0x21A0,
    ConfigCls1GpioPulseDurationPei6 = 0x21B0,
    ConfigCls1GpioPulseDurationPei7 = 0x21C0,
    ConfigCls1GpioPulseDurationPb0 = 0x21D0,
    ConfigCls1GpioPulseDurationPe0 = 0x21E0,
    ConfigCls1GpioPulseDurationPe1 = 0x21F0,
    ConfigCls1GpioPulseDurationReserved = 0x2200,
    ConfigCls1GpioDirections = 0x2210,
    ConfigCls1GpioOutputsDefault = 0x2220,
    ConfigCls1GpioMaskTelemetryOnChange = 0x2230,
    DiagResetCount = 0x3000,
    DiagLastResetReason = 0x3010,
    DiagResetCountUnknown = 0x3020,
    DiagResetCountPowerOn = 0x3030,
    DiagResetCountWatchdog = 0x3040,
    DiagResetCountExternal = 0x3050,
}

impl TryFrom<u16> for Attribute {
    type Error = ProtocolError;

    fn try_from(mut value: u16) -> Result<Self, Self::Error> {
        // 4 lsb are the attribute part
        value &= 0xfff0;

        match num_traits::FromPrimitive::from_u16(value) {
            Some(attr) => Ok(attr),
            None => Err(ProtocolError::UnknownAttributeKey),
        }
    }
}
