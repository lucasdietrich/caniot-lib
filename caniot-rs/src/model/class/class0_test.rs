use crate::{
    Temperature,
    class::{
        TempSensType,
        llpayload::{LLCommand, LLTelemetry},
    },
    datatypes::Xps,
};

use super::class0::*;

#[test]
fn telemetry_ios() {
    let mut t = Telemetry::default();
    t.set_io(IO::Oc1, true).unwrap();
    t.set_io(IO::Oc2, false).unwrap();
    t.set_io(IO::Relay1, true).unwrap();
    t.set_io(IO::Relay2, false).unwrap();
    t.set_io(IO::Input1, true).unwrap();
    t.set_io(IO::Input2, false).unwrap();
    t.set_io(IO::Input3, true).unwrap();
    t.set_io(IO::Input4, false).unwrap();
    t.set_io(IO::Oc1PulseActive, true).unwrap();
    t.set_io(IO::Oc2PulseActive, false).unwrap();
    t.set_io(IO::Relay1PulseActive, true).unwrap();
    t.set_io(IO::Relay2PulseActive, false).unwrap();

    assert_eq!(t.get_io(IO::Oc1), Ok(true));
    assert_eq!(t.get_io(IO::Oc2), Ok(false));
    assert_eq!(t.get_io(IO::Relay1), Ok(true));
    assert_eq!(t.get_io(IO::Relay2), Ok(false));
    assert_eq!(t.get_io(IO::Input1), Ok(true));
    assert_eq!(t.get_io(IO::Input2), Ok(false));
    assert_eq!(t.get_io(IO::Input3), Ok(true));
    assert_eq!(t.get_io(IO::Input4), Ok(false));
    assert_eq!(t.get_io(IO::Oc1PulseActive), Ok(true));
    assert_eq!(t.get_io(IO::Oc2PulseActive), Ok(false));
    assert_eq!(t.get_io(IO::Relay1PulseActive), Ok(true));
    assert_eq!(t.get_io(IO::Relay2PulseActive), Ok(false));
}

#[test]
fn telemetry_temperature() {
    let temp = 25.0;

    let mut t = Telemetry::default();
    t.set_temperature(TempSensType::BoardSensor, Temperature::from_celsius(temp))
        .expect("Failed to set temperature");
    t.set_temperature(
        TempSensType::ExternalSensor(0),
        Temperature::from_celsius(temp + 1.0),
    )
    .expect("Failed to set temperature");
    t.set_temperature(
        TempSensType::ExternalSensor(1),
        Temperature::from_celsius(temp + 2.0),
    )
    .expect("Failed to set temperature");
    t.set_temperature(
        TempSensType::ExternalSensor(2),
        Temperature::from_celsius(temp + 3.0),
    )
    .expect("Failed to set temperature");
    let read_temp = t
        .get_temperature(TempSensType::BoardSensor)
        .expect("Failed to get temperature");
    assert_eq!(Temperature::from_celsius(temp), read_temp);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(0))
        .expect("Failed to get temperature");
    assert_eq!(Temperature::from_celsius(temp + 1.0), read_temp);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(1))
        .expect("Failed to get temperature");
    assert_eq!(Temperature::from_celsius(temp + 2.0), read_temp);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(2))
        .expect("Failed to get temperature");
    assert_eq!(Temperature::from_celsius(temp + 3.0), read_temp);
}

#[test]
fn command_ios() {
    let mut c = Command::default();
    c.set_io_xps(IO::Oc1, Xps::PulseOn)
        .expect("Failed to set IO XPS");
    c.set_io_xps(IO::Oc2, Xps::Reset)
        .expect("Failed to set IO XPS");
    c.set_io_xps(IO::Relay1, Xps::Toggle)
        .expect("Failed to set IO XPS");
    c.set_io_xps(IO::Relay2, Xps::None)
        .expect("Failed to set IO XPS");

    assert_eq!(c.get_io_xps(IO::Oc1).unwrap(), Xps::PulseOn);
    assert_eq!(c.get_io_xps(IO::Oc2).unwrap(), Xps::Reset);
    assert_eq!(c.get_io_xps(IO::Relay1).unwrap(), Xps::Toggle);
    assert_eq!(c.get_io_xps(IO::Relay2).unwrap(), Xps::None);
}
