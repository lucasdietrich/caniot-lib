use std::os::fd::AsFd;

use caniot::{
    device::{Device, StaticConfig},
    driver::LinuxDriver,
    types::Endpoint,
};
use caniot_emu::garage::GarageController;
use log::{debug, error, info, warn};
use nix::poll::{PollFd, PollFlags, PollTimeout, poll};

const DEV: &str = "vcan0";

fn main() {
    simple_logger::init_with_level(log::Level::Info).unwrap();

    let sensor1 = GarageController::default();
    let config1 = StaticConfig::new();
    let driver1 = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

    let mut device1 =
        Device::init(driver1, 8, sensor1, config1).expect("Failed to initialize device");
    device1.request_endpoint_telemetry(Endpoint::BoardControl);

    let handle = std::thread::spawn(move || {
        loop {
            let timeout = [device1.next_timeout()]
                .into_iter()
                .flatten()
                .min()
                .map(|d| {
                    d.try_into()
                        .expect("Failed to convert duration to PollTimeout")
                })
                .unwrap_or(PollTimeout::NONE);
            let pollfd1 = PollFd::new(device1.as_fd(), PollFlags::POLLIN);
            let mut fds = [pollfd1];
            debug!("Polling {:?} with timeout {:?}", fds, timeout);
            match poll(&mut fds, timeout) {
                Ok(n) => {
                    let fd1_ready = fds[0]
                        .revents()
                        .unwrap_or(PollFlags::empty())
                        .contains(PollFlags::POLLIN);

                    if fd1_ready || n == 0 {
                        match device1.run_once() {
                            Ok(()) => {}
                            Err(err) => {
                                if !err.is_eagain() {
                                    error!("Device 1 error: {}", err);
                                } else {
                                    warn!("Device 1 non-fatal error: {}", err);
                                }
                            }
                        }
                    }
                }
                Err(err) => {
                    error!("Poll error: {}", err);
                }
            };
        }
    });

    handle.join().expect("Thread panicked");
}
