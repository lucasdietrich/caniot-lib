use core::{mem::MaybeUninit, time::Duration};
#[cfg(feature = "std")]
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, RawFd};

use crate::{
    device::{
        api::DeviceApiWrapper, config::StaticConfig, identity::Identity, implementation::DeviceApi,
    },
    did::DeviceId,
    driver::Driver,
    error::FailCode,
    types::Endpoint,
};
use caniot_sys as ll;

#[allow(dead_code)]
pub struct Device<D: Driver, A: DeviceApi + 'static> {
    device: ll::caniot_device,
    config: Box<StaticConfig>,
    identity: Identity,
    api: DeviceApiWrapper<A>,
    driver: D,
}

unsafe impl<D: Driver, A: DeviceApi + 'static> Send for Device<D, A> {}
unsafe impl<D: Driver, A: DeviceApi + 'static> Sync for Device<D, A> {}

impl<D: Driver, A: DeviceApi + 'static> Device<D, A> {
    pub fn init(
        mut driver: D,
        did: DeviceId,
        api: A,
        config: StaticConfig,
    ) -> Result<Device<D, A>, FailCode> {
        let mut device: MaybeUninit<ll::caniot_device> = MaybeUninit::uninit();
        let mut config = Box::new(config);
        let identity = Identity::new(did);
        let mut api = DeviceApiWrapper::new(api);
        let ret = unsafe {
            ll::caniot_device_init(
                device.as_mut_ptr(),
                identity.as_ref(),
                api.get_api_vtable().as_ptr(),
                api.get_api_data(),
                config.as_mut().as_mut(),
                driver.get_api().as_ptr(),
                driver.get_data(),
            )
        };
        let device = unsafe { device.assume_init() };
        FailCode::to_result(ret)?;
        Ok(Device {
            device,
            config,
            identity,
            api,
            driver,
        })
    }

    #[allow(dead_code)]
    fn as_ptr(&self) -> *const ll::caniot_device {
        &self.device
    }

    pub fn run_once(&mut self) -> Result<(), FailCode> {
        let ret = unsafe { ll::caniot_device_process(&mut self.device) };
        FailCode::to_result(ret)?;
        Ok(())
    }

    pub fn next_timeout(&mut self) -> Option<Duration> {
        let ret = unsafe { ll::caniot_device_time_until_process(&mut self.device) };
        match ret {
            u32::MAX => None,
            timeout => Some(Duration::from_millis(timeout as u64)),
        }
    }

    pub fn request_endpoint_telemetry(&mut self, endpoint: Endpoint) {
        let endpoint: ll::caniot_endpoint_t::Type = endpoint.into();
        unsafe { ll::caniot_device_trigger_telemetry_ep(&mut self.device, endpoint) };
    }


    pub fn get_api_mut(&mut self) -> &mut A {
        self.api.as_mut()
    }
}

#[cfg(feature = "std")]
impl<D: Driver + AsRawFd, A: DeviceApi + 'static> AsRawFd for Device<D, A> {
    fn as_raw_fd(&self) -> RawFd {
        self.driver.as_raw_fd()
    }
}
#[cfg(feature = "std")]
impl<D: Driver + AsFd, A: DeviceApi + 'static> AsFd for Device<D, A> {
    fn as_fd(&self) -> BorrowedFd<'_> {
        self.driver.as_fd()
    }
}
