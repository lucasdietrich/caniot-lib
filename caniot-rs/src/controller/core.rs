// Core controller implementation (moved from controller.rs to avoid module_inception lint)

// Documentation:
// Read: https://github.com/lucasdietrich/zephyr-caniot-controller/blob/main/src/ha/caniot_controller.c

use core::mem::MaybeUninit;
use core::time::Duration;
#[cfg(feature = "std")]
use std::os::fd::{AsFd, AsRawFd};

use caniot_sys as ll;

use crate::controller::api::ControllerApiWrapper;
use crate::controller::event::ControllerEvent;
use crate::controller::implementation::{ControllerApi, EventVerdict};
use crate::controller::query::Handle;
use crate::did::DeviceId;
use crate::driver::Driver;
use crate::error::FailCode;
use crate::frame::{OwnedFrame, Request};

pub struct Controller<D: Driver> {
    controller: Box<ll::caniot_controller>,
    driver: D,
    _api: ControllerApiWrapper,
}

unsafe impl<D: Driver> Send for Controller<D> {}
unsafe impl<D: Driver> Sync for Controller<D> {}

#[unsafe(no_mangle)]
unsafe extern "C" fn trampoline_controler_cb(
    ev: *const ll::caniot_controller_event_t,
    user_data: *mut ::core::ffi::c_void,
) -> bool {
    debug_assert!(!ev.is_null());

    let api_data = unsafe { &mut *(user_data as *mut Box<dyn ControllerApi>) };
    let api = api_data.as_mut();
    let event = unsafe { ControllerEvent::from_ll_unchecked(ev) };
    match api.on_event(event) {
        EventVerdict::Continue => true,
        EventVerdict::Abort => false,
    }
}

impl<D: Driver> Controller<D> {
    pub fn init<A: ControllerApi + 'static>(
        mut driver: D,
        api: A,
    ) -> Result<Controller<D>, FailCode> {
        let mut controller: Box<MaybeUninit<ll::caniot_controller>> =
            Box::new(MaybeUninit::uninit());
        let ptr = controller.as_mut_ptr();
        let mut api = ControllerApiWrapper::new(api);

        let ret = unsafe {
            ll::caniot_controller_driv_init(
                ptr,
                driver.get_api().as_ptr(),
                driver.get_data(),
                Some(trampoline_controler_cb),
                api.get_data(),
                ll::CANIOT_CONTROLLER_FLAG_NONE as u8,
            )
        };
        FailCode::to_result(ret)?;

        Ok(Controller {
            controller: unsafe { controller.assume_init() },
            driver,
            _api: api,
        })
    }

    fn as_mut_ptr(&mut self) -> *mut ll::caniot_controller {
        self.controller.as_mut()
    }
    fn as_ptr(&self) -> *const ll::caniot_controller {
        self.controller.as_ref()
    }

    pub fn next_timeout(&self) -> Option<Duration> {
        let ret = unsafe { ll::caniot_controller_next_timeout(self.as_ptr()) };
        match ret {
            u32::MAX => None,
            timeout => Some(Duration::from_millis(timeout as u64)),
        }
    }

    pub fn run(&mut self) -> Result<(), FailCode> {
        let ret = unsafe { ll::caniot_controller_process(self.as_mut_ptr()) };
        FailCode::to_result(ret)?;
        Ok(())
    }

    pub fn query(
        &mut self,
        did: DeviceId,
        request: Request,
        timeout: Option<Duration>,
    ) -> Result<Option<Handle>, FailCode> {
        let timeout_ms = timeout.unwrap_or(Duration::from_millis(0)).as_millis() as u32;
        let mut frame = OwnedFrame::new_request(request);
        let ret = unsafe {
            ll::caniot_controller_query(self.as_mut_ptr(), did.to_u8(), frame.as_mut(), timeout_ms)
        };
        FailCode::to_result(ret).map(|handle| Handle::new(handle as u8))
    }
}

impl<D: Driver> Drop for Controller<D> {
    fn drop(&mut self) {
        unsafe { ll::caniot_controller_deinit(self.as_mut_ptr()) };
    }
}

#[cfg(feature = "std")]
impl<D: Driver + AsRawFd> AsRawFd for Controller<D> {
    fn as_raw_fd(&self) -> std::os::fd::RawFd {
        self.driver.as_raw_fd()
    }
}

#[cfg(feature = "std")]
impl<D: Driver + AsFd> AsFd for Controller<D> {
    fn as_fd(&self) -> std::os::fd::BorrowedFd<'_> {
        self.driver.as_fd()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{controller::implementation::DummyControllerApi, driver::DummyDriver};
    #[test]
    fn test_controller_init() {
        let ctrl = Controller::init(DummyDriver, DummyControllerApi);
        assert!(ctrl.is_ok());
    }
}
