use num::FromPrimitive;

use super::datatypes::{TS, TSP};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct SysCtrl {
    pub hardware_reset: bool,
    pub _software_reset: bool, // deprecated
    pub _watchdog_reset: bool, // deprecated
    pub watchdog_enable: TS,
    pub factory_reset: bool,
    pub inhibit: TSP,
}

impl SysCtrl {
    pub const HARDWARE_RESET: SysCtrl = SysCtrl {
        hardware_reset: true,
        _software_reset: false,
        _watchdog_reset: false,
        watchdog_enable: TS::None,
        factory_reset: false,
        inhibit: TSP::None,
    };

    pub const FACTORY_RESET: SysCtrl = SysCtrl {
        hardware_reset: false,
        _software_reset: false,
        _watchdog_reset: false,
        watchdog_enable: TS::None,
        factory_reset: true,
        inhibit: TSP::None,
    };

    // // Converts the SysCtrl into a class command request
    // pub fn into_board_request(self) -> RequestData {
    //     RequestData::new_board_control_request(self)
    // }

    pub const fn inhibit_control(inhibit: TSP) -> SysCtrl {
        SysCtrl {
            hardware_reset: false,
            _software_reset: false,
            _watchdog_reset: false,
            watchdog_enable: TS::None,
            factory_reset: false,
            inhibit,
        }
    }
}

impl From<SysCtrl> for u8 {
    fn from(val: SysCtrl) -> Self {
        let mut payload = 0_u8;

        payload |= val.hardware_reset as u8;
        payload |= (val._software_reset as u8) << 1;
        payload |= (val._watchdog_reset as u8) << 2;
        payload |= (val.watchdog_enable as u8) << 3;
        payload |= (val.factory_reset as u8) << 5;
        payload |= (val.inhibit as u8) << 6;

        payload
    }
}

impl From<u8> for SysCtrl {
    fn from(value: u8) -> Self {
        SysCtrl {
            hardware_reset: value & 0b0000_0001 != 0,
            _software_reset: value & 0b0000_0010 != 0,
            _watchdog_reset: value & 0b0000_0100 != 0,
            watchdog_enable: FromPrimitive::from_u8((value & 0b0001_1000) >> 2).unwrap(),
            factory_reset: value & 0b0010_0000 != 0,
            inhibit: FromPrimitive::from_u8((value & 0b1100_0000) >> 6).unwrap(),
        }
    }
}
