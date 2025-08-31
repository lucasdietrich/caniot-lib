use core::{mem::MaybeUninit, time::Duration};

#[cfg(feature = "std")]
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, RawFd};

use crate::{
    device::{
        api::DeviceApiWrapper, config::StaticConfig, identity::Identity, implementation::DeviceApi,
    },
    driver::Driver,
    error::FailCode,
    types::Endpoint,
};
use caniot_sys as ll;

pub struct Device<D: Driver> {
    device: ll::caniot_device,
    config: Box<StaticConfig>,
    identity: Identity,
    api: DeviceApiWrapper, // addr shouldn't move, device has pointers which reference api handlers
    driver: D,
}

unsafe impl<D: Driver> Send for Device<D> {}
unsafe impl<D: Driver> Sync for Device<D> {}

impl<D: Driver> Device<D> {
    pub fn init<A: DeviceApi + 'static>(
        mut driver: D,
        did: u8,
        api: A,
        config: StaticConfig,
    ) -> Result<Device<D>, FailCode> {
        let mut device: MaybeUninit<ll::caniot_device> = MaybeUninit::uninit();
        let mut config = Box::new(config);
        let identity = Identity::new(did);
        let mut api = DeviceApiWrapper::new(api);
        let ret = unsafe {
            ll::caniot_device_init(
                device.as_mut_ptr(),
                identity.as_ref(),
                api.get_api().as_ptr(),
                api.get_data(),
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

    // pub fn as_mut(&mut self) -> &mut ll::caniot_device {
    //     &mut self.device
    // }

    pub fn as_ptr(&self) -> *const ll::caniot_device {
        &self.device
    }

    pub fn run_once(&mut self) -> Result<(), FailCode> {
        let ret = unsafe { ll::caniot_device_process(&mut self.device) };
        FailCode::to_result(ret)?;
        Ok(())
    }

    pub fn next_timeout(&mut self) -> Option<Duration> {
        // SAFETY: We are calling a method that does not mutate the device.
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

    pub fn run(&mut self) -> Result<(), FailCode> {
        loop {
            self.run_once()?;
        }
    }
}

#[cfg(feature = "std")]
impl<D: Driver + AsRawFd> AsRawFd for Device<D> {
    fn as_raw_fd(&self) -> RawFd {
        self.driver.as_raw_fd()
    }
}

#[cfg(feature = "std")]
impl<D: Driver + AsFd> AsFd for Device<D> {
    fn as_fd(&self) -> BorrowedFd<'_> {
        self.driver.as_fd()
    }
}
