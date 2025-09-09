use crate::{class::{llpayload::{LLCommand, LLTelemetry}, TempSensType}, datatypes::Xps};

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

    assert_eq!(t.get_io(IO::Oc1), Some(true));
    assert_eq!(t.get_io(IO::Oc2), Some(false));
    assert_eq!(t.get_io(IO::Relay1), Some(true));
    assert_eq!(t.get_io(IO::Relay2), Some(false));
    assert_eq!(t.get_io(IO::Input1), Some(true));
    assert_eq!(t.get_io(IO::Input2), Some(false));
    assert_eq!(t.get_io(IO::Input3), Some(true));
    assert_eq!(t.get_io(IO::Input4), Some(false));
    assert_eq!(t.get_io(IO::Oc1PulseActive), Some(true));
    assert_eq!(t.get_io(IO::Oc2PulseActive), Some(false));
    assert_eq!(t.get_io(IO::Relay1PulseActive), Some(true));
    assert_eq!(t.get_io(IO::Relay2PulseActive), Some(false));
}

#[test]
fn telemetry_temperature() {
    let temp = 25.0;

    let mut t = Telemetry::default();
    t.set_temperature(TempSensType::BoardSensor, temp)
        .expect("Failed to set temperature");
    t.set_temperature(TempSensType::ExternalSensor(0), temp + 1.0)
        .expect("Failed to set temperature");
    t.set_temperature(TempSensType::ExternalSensor(1), temp + 2.0)
        .expect("Failed to set temperature");
    t.set_temperature(TempSensType::ExternalSensor(2), temp + 3.0)
        .expect("Failed to set temperature");
    let read_temp = t
        .get_temperature(TempSensType::BoardSensor)
        .expect("Failed to get temperature");
    assert!((read_temp - temp).abs() < f32::EPSILON);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(0))
        .expect("Failed to get temperature");
    assert!((read_temp - (temp + 1.0)).abs() < f32::EPSILON);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(1))
        .expect("Failed to get temperature");
    assert!((read_temp - (temp + 2.0)).abs() < f32::EPSILON);
    let read_temp = t
        .get_temperature(TempSensType::ExternalSensor(2))
        .expect("Failed to get temperature");
    assert!((read_temp - (temp + 3.0)).abs() < f32::EPSILON);
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
