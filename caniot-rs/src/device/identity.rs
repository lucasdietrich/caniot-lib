use caniot_sys as ll;

pub struct Identity {
    identification: Box<ll::caniot_device_id>,
}

impl Identity {
    pub fn new(did: u8) -> Self {
        let identification = ll::caniot_device_id {
            did,
            ..unsafe { core::mem::zeroed() }
        };
        let identification = Box::new(identification);
        Self { identification }
    }

    pub fn as_ref(&self) -> &ll::caniot_device_id {
        &self.identification
    }
}
