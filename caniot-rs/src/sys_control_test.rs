use crate::datatypes::TSP;

use super::sys_control::*;

#[test]
fn sys_default() {
    let sys = SysCtrl::default();
    let sys_ser: u8 = sys.into();

    assert_eq!(sys_ser, 0_u8);
}

#[test]
fn sys_hardware_reset() {
    let mut sys = SysCtrl::HARDWARE_RESET;
    assert_eq!(sys.hardware_reset, true);

    let sys_ser: u8 = sys.into();
    assert_eq!(sys_ser, 1_u8);

    sys.hardware_reset = false;
    let sys_ser: u8 = sys.into();
    assert_eq!(sys_ser, 0_u8);
}

#[test]
fn sys_factory_reset() {
    let mut sys = SysCtrl::FACTORY_RESET;
    assert_eq!(sys.factory_reset, true);

    let sys_ser: u8 = sys.into();
    assert_eq!(sys_ser, 32_u8);

    sys.factory_reset = false;
    let sys_ser: u8 = sys.into();
    assert_eq!(sys_ser, 0_u8);
}

#[test]
fn sys_inhibit_control() {
    fn test_mode(mode: TSP) {
        let mut sys = SysCtrl::inhibit_control(mode);
        assert_eq!(sys.inhibit, mode);

        let mode_number = mode as u8;

        let sys_ser: u8 = sys.into();
        assert_eq!(sys_ser >> 6, mode_number);

        sys.inhibit = TSP::None;
        let sys_ser: u8 = sys.into();
        assert_eq!(sys_ser, 0_u8);
    }

    test_mode(TSP::None);
    test_mode(TSP::Set);
    test_mode(TSP::Reset);
    test_mode(TSP::Pulse);
}
