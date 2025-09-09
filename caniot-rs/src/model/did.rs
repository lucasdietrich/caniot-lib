use core::fmt;

use crate::error::FailCode;

#[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct DeviceId {
    pub class: u8,
    pub sub_id: u8,
}

impl TryFrom<u8> for DeviceId {
    type Error = FailCode;

    fn try_from(id: u8) -> Result<Self, Self::Error> {
        if id > 0x3f {
            return Err(FailCode::EINVAL);
        } else {
            Ok(unsafe { DeviceId::new_from_raw_unchecked(id) })
        }
    }
}

impl DeviceId {
    pub const BROADCAST: DeviceId = DeviceId {
        class: 0x7,
        sub_id: 0x7,
    };

    pub(crate) const unsafe fn new_unchecked(class: u8, sub_id: u8) -> Self {
        DeviceId { class, sub_id }
    }

    pub const fn new(class: u8, sub_id: u8) -> Result<Self, FailCode> {
        if class > 0x7 || sub_id > 0x7 {
            Err(FailCode::EINVAL)
        } else {
            Ok(unsafe { DeviceId::new_unchecked(class, sub_id) })
        }
    }

    pub(crate) unsafe fn new_from_raw_unchecked(did: u8) -> Self {
        unsafe { Self::new_unchecked(did & 0x7, (did >> 3) & 0x7) }
    }

    pub fn to_u8(&self) -> u8 {
        (self.sub_id << 3) | self.class
    }

    pub fn is_broadcast(&self) -> bool {
        self == &Self::BROADCAST
    }
}

impl fmt::Display for DeviceId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "({}: {},{})", self.to_u8(), self.class, self.sub_id)
    }
}

impl fmt::Debug for DeviceId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "DeviceId{}", self)
    }
}
