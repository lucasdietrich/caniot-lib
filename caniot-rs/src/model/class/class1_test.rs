use crate::{class::{llpayload::{LLCommand, LLTelemetry}, TempSensType}, datatypes::Xps};

use super::class1::*;

#[test]
fn telemetry_ios() {
    let mut t = Telemetry::default();

    let states: &[(IO, bool)] = &[
        (IO::Pc0, true),
        (IO::Pc1, false),
        (IO::Pc2, true),
        (IO::Pc3, false),
        (IO::Pd4, true),
        (IO::Pd5, false),
        (IO::Pd6, true),
        (IO::Pd7, false),
        (IO::Eio0, true),
        (IO::Eio1, false),
        (IO::Eio2, true),
        (IO::Eio3, false),
        (IO::Eio4, true),
        (IO::Eio5, false),
        (IO::Eio6, true),
        (IO::Eio7, false),
        (IO::Pb0, true),
        (IO::Pe0, false),
        (IO::Pe1, true),
    ];

    for (io, state) in states {
        t.set_io(*io, *state).unwrap();
    }

    for (io, state) in states {
        assert_eq!(t.get_io(*io), Some(*state), "Mismatch on {:?}", io);
    }
}

#[test]
fn telemetry_temperature() {
    let base = 21.5;
    let mut t = Telemetry::default();

    t.set_temperature(TempSensType::BoardSensor, base).unwrap();
    t.set_temperature(TempSensType::ExternalSensor(0), base + 1.0)
        .unwrap();
    t.set_temperature(TempSensType::ExternalSensor(1), base + 2.0)
        .unwrap();
    t.set_temperature(TempSensType::ExternalSensor(2), base + 3.0)
        .unwrap();

    let eps = f32::EPSILON;
    assert!((t.get_temperature(TempSensType::BoardSensor).unwrap() - base).abs() < eps);
    assert!(
        (t.get_temperature(TempSensType::ExternalSensor(0)).unwrap() - (base + 1.0)).abs() < eps
    );
    assert!(
        (t.get_temperature(TempSensType::ExternalSensor(1)).unwrap() - (base + 2.0)).abs() < eps
    );
    assert!(
        (t.get_temperature(TempSensType::ExternalSensor(2)).unwrap() - (base + 3.0)).abs() < eps
    );
}

#[test]
fn command_ios() {
    let mut c = Command::default();

    c.set_io_xps(IO::Pc0, Xps::PulseOn).unwrap();
    c.set_io_xps(IO::Pc1, Xps::Reset).unwrap();
    c.set_io_xps(IO::Pd4, Xps::Toggle).unwrap();
    c.set_io_xps(IO::Eio0, Xps::None).unwrap();
    c.set_io_xps(IO::Eio7, Xps::PulseOn).unwrap();
    c.set_io_xps(IO::Pe1, Xps::Toggle).unwrap();

    assert_eq!(c.get_io_xps(IO::Pc0).unwrap(), Xps::PulseOn);
    assert_eq!(c.get_io_xps(IO::Pc1).unwrap(), Xps::Reset);
    assert_eq!(c.get_io_xps(IO::Pd4).unwrap(), Xps::Toggle);
    assert_eq!(c.get_io_xps(IO::Eio0).unwrap(), Xps::None);
    assert_eq!(c.get_io_xps(IO::Eio7).unwrap(), Xps::PulseOn);
}
