use caniot_sys as ll;

use crate::did::DeviceId;

pub struct Identity {
    identification: Box<ll::caniot_device_id>,
}

impl Identity {
    pub fn new(did: DeviceId) -> Self {
        let identification = ll::caniot_device_id {
            did: did.to_u8(),
            ..unsafe { core::mem::zeroed() }
        };
        let identification = Box::new(identification);
        Self { identification }
    }

    pub fn as_ref(&self) -> &ll::caniot_device_id {
        &self.identification
    }
}
