use caniot_sys as ll;

use crate::types::Endpoint;

pub struct StaticConfig {
    config: ll::caniot_device_config,
}

impl StaticConfig {
    pub fn new() -> Self {
        let mut config: ll::caniot_device_config = unsafe { core::mem::zeroed() };

        config.telemetry.period = 30000;
        config.telemetry.__bindgen_anon_1.delay_min = 0;
        config.telemetry.delay_max = 100;

        config.flags.set_error_response(1);
        config.flags.set_telemetry_delay_rdm(1);

        config
            .flags
            .set_telemetry_endpoint(Endpoint::ApplicationDefault.into());
        config.flags.set_telemetry_periodic_enabled(1);

        config.timezone = 3600;
        config.location.region = ['E' as i8, 'U' as i8];
        config.location.country = ['F' as i8, 'R' as i8];

        Self { config }
    }

    pub fn zeroed() -> Self {
        let config: ll::caniot_device_config = unsafe { core::mem::zeroed() };

        Self { config }
    }

    pub fn as_mut(&mut self) -> &mut ll::caniot_device_config {
        &mut self.config
    }
}
